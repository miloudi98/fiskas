#include <iterator>
#include <span>
#include <functional>
#include <algorithm>
#include <optional>
#include <type_traits>
#include <utility>
#include <filesystem>
#include <vector>

#include "base.hh"
#include "fmt/format.h"
#include "assembler/elf.hh"

using fiska::elf::ElfHeader;

#define ELF64_ST_INFO(bind, type) (((bind)<<4)+((type)&0xf))
#define STB_GLOBAL 1
#define STV_DEFAULT
#define STT_FUNC 2

namespace detail {

template <typename T, typename... U>
concept is_any_of = (... or std::same_as<T, U>);

template <typename T>
void extend(std::vector<T> &dst, const std::vector<T> &src) {
	std::copy(src.begin(), src.end(), std::back_inserter(dst));
}

} // namespace detail

using ByteVec = std::vector<u8>;

enum struct SectionType {
	SectionHeaderStringTable,
	Null,
	Text,
	SymTab,
	SymTabStringTable,
	Invalid
};

enum struct ElfHeaderRelocationLabel {
	SectionHeaderTableOffset,
	SectionHeaderStringTableIdx,
	NumberOfSectionHeaders,
	Invalid
};

struct SectionRelocationLabel {
	enum struct Label {
		SectionName,
		SectionOffset,
		SectionSize,
		Invalid
	};

	SectionType section_type = SectionType::Invalid;
	Label label = Label::Invalid;

public:
	auto operator==(const SectionRelocationLabel &rhs) const -> bool {
		return (section_type == rhs.section_type) and (label == rhs.label);
	}
};

struct Relocation {
	using LabelType = std::variant<ElfHeaderRelocationLabel, SectionRelocationLabel>;

	u64 offset{};
	LabelType label;
};

struct Elf {
    constexpr static u32 null_section_header_type = 0;
    constexpr static u32 string_table_section_header_type = 3;
};

struct SectionHeader {
    u32 sh_name{};
    u32 sh_type{};
    u64 sh_flags{};
    u64 sh_addr{};
    u64 sh_offset{};
    u64 sh_size{};
    u32 sh_link{};
    u32 sh_info{};
    u64 sh_addralign{};
    u64 sh_entsize{};
};

struct Section {
    std::string name{};
    SectionHeader header{};
    ByteVec body{};
    SectionType type = SectionType::Invalid;
};

struct Symbol {
	u32 st_name{};
	u8 st_info{};
	u8 st_other{};
	u16 st_shndx{};
	u64 st_value{};
	u64 st_size{};
};


struct Serializer {
	using RelocationVec = std::vector<Relocation>;
	using RelocationFixVec = std::vector<std::function<void()>>;

	ByteVec out;
	RelocationVec relocations;
	RelocationFixVec relocation_fixes;

public:
	template <typename T>
	auto fix_relocation(Relocation::LabelType label, T value) {
		auto fix = [this, label, value]() {
			Serializer ser; ser << value;

			auto reloc = std::find_if(relocations.begin(), relocations.end(), 
					[&](const Relocation &reloc) {
						return reloc.label == label;
					}
			);
			if (reloc == std::end(relocations)) {
				fmt::print("Failed to find relocation\n");
				std::exit(1);
			}

			for (u32 idx = 0; idx < ser.out.size(); ++idx) {
				out[reloc->offset + idx] = ser.out[idx];
			}
		};
		relocation_fixes.push_back(fix);
	}

	auto dump_to(const fs::path &path) {
		for (const auto &relocation_fix : relocation_fixes) { 
			relocation_fix();
		}
		File::write(out.data(), out.size(), path);
	}

public:
	template <typename T>
	requires ::detail::is_any_of<T, u8, u16, u32, u64>
	auto operator<<(T value) -> Serializer & {
		out.reserve(out.size() + sizeof value);
		for (u32 idx = 0; idx < sizeof value; ++idx) {
			out.push_back(value & 0xffu);
			value >>= 8;
		}
		return *this;
	}

	template <typename T, u32 sz>
	requires std::same_as<std::remove_cvref_t<T>, u8>
	auto operator<<(T (&arr)[sz]) -> Serializer & {
		out.reserve(out.size() + sz);
		for (u32 idx = 0; idx < sz; ++idx) {
			out.push_back(arr[idx]);
		}
		return *this;
	}

	auto operator<<(const ByteVec &vec) -> Serializer & {
		out.reserve(out.size() + vec.size());
		std::copy(vec.begin(), vec.end(), std::back_inserter(out));
		return *this;
	}

	auto operator<<(std::span<Section> section_header_table) -> Serializer & {
		fix_relocation(ElfHeaderRelocationLabel::NumberOfSectionHeaders, static_cast<u16>(section_header_table.size()));
		fix_relocation(ElfHeaderRelocationLabel::SectionHeaderTableOffset, static_cast<u64>(out.size()));

		u64 sh_strtab_idx = [&] {
			auto sh_strtab_it = std::find_if(section_header_table.begin(), section_header_table.end(),
					[&](const Section &section) {
						return section.type == SectionType::SectionHeaderStringTable;
					}
			);

			if (sh_strtab_it == std::end(section_header_table)) {
				fmt::print("Failed to find the section header strign table\n");
				std::exit(1);
			}

			return static_cast<u64>(sh_strtab_it - section_header_table.begin());
		}();
		fix_relocation(ElfHeaderRelocationLabel::SectionHeaderStringTableIdx, static_cast<u16>(sh_strtab_idx));

		// Serializer the section headers.
		for (const auto &section : section_header_table) {
			Relocation name_reloc = {
				.offset = out.size(),
				.label = (SectionRelocationLabel) {
					.section_type = section.type,
					.label = SectionRelocationLabel::Label::SectionName
				}
			};
			relocations.push_back(name_reloc);
			*this << section.header.sh_name;

			*this << section.header.sh_type
				  << section.header.sh_flags
				  << section.header.sh_addr;

			Relocation offset_reloc = {
				.offset = out.size(),
				.label = (SectionRelocationLabel) {
					.section_type = section.type,
					.label = SectionRelocationLabel::Label::SectionOffset
				}
			};
			relocations.push_back(offset_reloc);
			*this << section.header.sh_offset;

			Relocation size_reloc = {
				.offset = out.size(),
				.label = (SectionRelocationLabel) {
					.section_type = section.type,
					.label = SectionRelocationLabel::Label::SectionSize
				}
			};
			relocations.push_back(size_reloc);
			*this << section.header.sh_size;

			*this << section.header.sh_link
				  << section.header.sh_info
				  << section.header.sh_addralign
				  << section.header.sh_entsize;
		}

		// Serialize the section bodies.
		auto serialize_section = [&](const Section &section) {
			u64 name_offset = [&] {
				u64 ret = section_header_table[sh_strtab_idx].body.size();
				::detail::extend(section_header_table[sh_strtab_idx].body, ByteVec(section.name.begin(), section.name.end()));
				return ret;
			}();

			Relocation::LabelType name_reloc = (SectionRelocationLabel) {
				.section_type = section.type,
				.label = SectionRelocationLabel::Label::SectionName
			};
			fix_relocation(name_reloc, static_cast<u32>(name_offset));

			Relocation::LabelType size_reloc = (SectionRelocationLabel) {
				.section_type = section.type,
				.label = SectionRelocationLabel::Label::SectionSize
			};
			fix_relocation(size_reloc, static_cast<u64>(section.body.size()));

			Relocation::LabelType offset_reloc = (SectionRelocationLabel) {
				.section_type = section.type,
				.label = SectionRelocationLabel::Label::SectionOffset
			};
			fix_relocation(offset_reloc, static_cast<u64>(out.size()));

			*this << section.body;
		};

		for (const auto &section : section_header_table) {
			// We are still updating the size of the section header string table.
			// The body of the section header string table will be serialized in the end.
			if (section.type == SectionType::SectionHeaderStringTable) continue;
			serialize_section(section);
		}

		serialize_section(section_header_table[sh_strtab_idx]);
		return *this;
	}

	auto operator<<(const ElfHeader &elf_header) -> Serializer & {
		*this << elf_header.e_ident
			  << elf_header.e_type
			  << elf_header.e_machine
			  << elf_header.e_version
			  << elf_header.e_entry
			  << elf_header.e_phoff;

		Relocation sh_tab_off = {
			.offset = out.size(),
			.label = ElfHeaderRelocationLabel::SectionHeaderTableOffset
		};
		relocations.push_back(sh_tab_off);
		*this << elf_header.e_shoff;

		*this << elf_header.e_flags
			  << elf_header.e_ehsize
			  << elf_header.e_phentsize
			  << elf_header.e_phnum
			  << elf_header.e_shentsize;

		Relocation num_sections = {
			.offset = out.size(),
			.label = ElfHeaderRelocationLabel::NumberOfSectionHeaders
		};
		relocations.push_back(num_sections);
		*this << elf_header.e_shnum;

		Relocation sh_strtab_idx = {
			.offset = out.size(),
			.label = ElfHeaderRelocationLabel::SectionHeaderStringTableIdx
		};
		relocations.push_back(sh_strtab_idx);
		*this << elf_header.e_shstrndx;

		return *this;
	}

	// Experimental
	auto operator<<(const Symbol &sym) -> Serializer & {
		*this << sym.st_name
			  << sym.st_info
			  << sym.st_other
			  << sym.st_shndx
			  << sym.st_value
			  << sym.st_size;
		return *this;
	}
};

struct SymbolSection {
	using SymbolVec = std::vector<Symbol>;

	std::unordered_map<std::string, u64> offsets;
	ByteVec out;
	SymbolVec symbols;

	auto add_string(std::string_view name) -> u64 {
		u64 ret = out.size();
		::detail::extend(out, ByteVec(name.begin(), name.end()));
		out.push_back(0x00);

		offsets[std::string(name)] = ret;
		return ret;
	}

	auto offset(std::string_view name) -> std::optional<u64> {
		if (offsets.contains(std::string(name))) {
			return offsets[std::string(name)];
		}
		return std::nullopt;
	}

	auto body() -> ByteVec {
		Serializer ser;
		// Null symbol
		symbols.push_back(Symbol{});
		out.push_back(0x00);

		Symbol sym = {
			.st_name = static_cast<u32>(add_string("test_function_1")),
			.st_info = ELF64_ST_INFO(STB_GLOBAL, STT_FUNC),
			.st_shndx = 1,
			.st_value = 0,
		};
		symbols.push_back(sym);

		for (const auto &sym : symbols) {
			ser << sym;
		}
		return ser.out;
	}

	// We are hard-coding the symbol here.
	SymbolSection() {

	}
};
// clang-format off
auto main(i32 argc, char *argv[]) -> i32 {
	Serializer serializer;

	std::vector<Section> section_header_table;
	ElfHeader header;
	Section null_section = {
		.name = "\0"s,
		.header = {
			.sh_type = SHT_NULL,
		},
		.type = SectionType::Null
	};
	Section sh_strtab = {
		.name = ".shstrtab\0"s,
		.header = {
			.sh_type = SHT_STRTAB,
			.sh_addralign = 1
		},
		.type = SectionType::SectionHeaderStringTable
	};
	Section text = {
		.name = ".text\0"s,
		.header = {
			.sh_type = SHT_PROGBITS,
			.sh_addralign = 1
		},
		.body = ByteVec{0x48, 0xc7, 0xc0, 0x7b, 0x00, 0x00, 0x00,0xc3},
		.type = SectionType::Text
	};

	SymbolSection sec;

	Section symtab = {
		.name = ".symtab\0"s,
		.header = {
			.sh_type = SHT_SYMTAB,
			.sh_link = 3,
			.sh_info = 1,
			.sh_addralign = 8,
			.sh_entsize = 24,
		},
		.body = sec.body(),
		.type = SectionType::SymTab
	};

	Section symtab_strtab = {
		.name = ".strtab\0"s,
		.header = {
			.sh_type = SHT_STRTAB,
			.sh_addralign = 1
		},
		.body = sec.out,
		.type = SectionType::SymTabStringTable
	};


	section_header_table.push_back(null_section);
	section_header_table.push_back(text);
	section_header_table.push_back(symtab);
	section_header_table.push_back(symtab_strtab);
	section_header_table.push_back(sh_strtab);

	serializer << header
		       << section_header_table;

	serializer.dump_to(fs::path("./elf_file_v2"));
    fmt::print("Bismillah\n");

    return 0;
}
// clang-format on
