#include <array>
#include <concepts>
#include <utility>
#include <algorithm>
#include <functional>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

#include "base.hh"
#include "fmt/format.h"

// remove these macros from here.
#define ELF64_ST_INFO(bind, type) (((bind)<<4)+((type)&0xf))
#define STB_GLOBAL 1
#define STV_DEFAULT
#define STT_FUNC 2

template <typename T, typename ...Option>
concept is_any_of = (... or std::same_as<T, Option>);

template <typename T>
concept UnsignedInt = is_any_of<T, u8, u16, u32, u64>;

using ByteVec = std::vector<u8>;

struct Utils {
	template <typename T>
	static auto extend(std::vector<T> &dst, const std::vector<T> &src) {
		std::copy(src.begin(), src.end(), std::back_inserter(dst));
	}
};

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
    SectionHeaderStrTabIdx,
    NumSectionHeaders,
    Invalid
};

struct SectionRelocationLabel {
    enum struct Label {
        SectionName,
        SectionOffset,
        SectionSize,
		TextSectionIdx,
		Link,
		Info,
        Invalid
    };

    SectionType section_type = SectionType::Invalid;
    Label label = Label::Invalid;
};

auto operator==(const SectionRelocationLabel &lhs, const SectionRelocationLabel &rhs) -> bool {
    return (lhs.section_type == rhs.section_type) and (lhs.label == rhs.label);
}

struct Relocation {
    // clang-format off
	using LabelType = std::variant<
		ElfHeaderRelocationLabel,
		SectionRelocationLabel
	>;
    // clang-format on

    std::optional<u64> offset = std::nullopt;
    LabelType label;

public:
    static auto with_label(LabelType label) -> Relocation {
        return (Relocation){.offset = std::nullopt, .label = label};
    }
};

struct RelocationFixInfo {
	std::optional<u64> value = std::nullopt;
	Relocation::LabelType label_to_fix;
	u16 value_size_in_bytes;
};

template <typename Value>
struct Entry {
    using ValueType = Value;
    using EntryType = std::variant<Value, Relocation>;

    EntryType entry;

    auto value() const -> Value { return std::get<Value>(entry); }

    auto relocation() const -> Relocation { return std::get<Relocation>(entry); }

	auto is_value() const -> bool { return std::holds_alternative<Value>(entry); }

	auto is_relocation() const -> bool { return std::holds_alternative<Relocation>(entry); }


    constexpr auto static serialized_size() -> u16 { return sizeof(std::remove_cvref_t<Value>); }

    template <typename T, u32 sz>
    constexpr static auto serialized_size()
        requires std::same_as<Value, std::array<T, sz>>
    {
        return sizeof(T) * sz;
    }
};

template <typename T>
concept is_entry = std::same_as<T, Entry<typename T::ValueType>>;

template <typename... Entries>
    requires(... and is_entry<Entries>)
constexpr auto entries_serialized_size(const Entries &...entries) -> u64 {
    return (... + entries.serialized_size());
}

struct ElfHeader {
    Entry<std::array<u8, 16>> e_ident{};
    Entry<u16> e_type;
    Entry<u16> e_machine;
    Entry<u32> e_version;
    Entry<u64> e_entry;
    Entry<u64> e_phoff;
    // Section header table offset.
    Entry<u64> e_shoff;
    Entry<u32> e_flags;
    Entry<u16> e_ehsize;
    Entry<u16> e_phentsize;
    Entry<u16> e_phnum;
    // Section header entry size.
    Entry<u16> e_shentsize;
    // Number of sections headers.
    Entry<u16> e_shnum;
    // Index of the section header string table in the section header table.
    Entry<u16> e_shstrndx;

public:
    constexpr static u8 elf_mag_0 = 0x7f;
    constexpr static u8 elf_mag_1 = 0x45;
    constexpr static u8 elf_mag_2 = 0x4c;
    constexpr static u8 elf_mag_3 = 0x46;
    constexpr static u8 elf_class_64 = 2;
    constexpr static u8 elf_data_2_lsb = 1;
    constexpr static u8 ev_current = 1;
    constexpr static u8 elf_os_abi_linux = 3;
    constexpr static u8 et_rel = 1;
    constexpr static u8 em_x86_64 = 62;

public:
    constexpr static auto serialized_size() -> u64 {
        ElfHeader header;
        // clang-format off
		return entries_serialized_size(
				header.e_ident, header.e_type, header.e_machine,
				header.e_version, header.e_entry, header.e_phoff,
				header.e_shoff, header.e_flags, header.e_ehsize,
				header.e_phentsize, header.e_phnum, header.e_shentsize,
				header.e_shnum, header.e_shstrndx
		);
        // clang-format on
    }
};

struct SectionHeader {
    Entry<u32> sh_name;
    Entry<u32> sh_type;
    Entry<u64> sh_flags;
    Entry<u64> sh_addr;
    Entry<u64> sh_offset;
    Entry<u64> sh_size;
    Entry<u32> sh_link;
    Entry<u32> sh_info;
    Entry<u64> sh_addralign;
    Entry<u64> sh_entsize;

public:
	constexpr static u8 sht_null = 0;
    constexpr static u8 sht_progbits = 1;
    constexpr static u8 sht_symtab = 2;
    constexpr static u8 sht_strtab = 3;

public:
	constexpr static auto name_reloc_label(SectionType sec_type) -> Relocation::LabelType {
		return (SectionRelocationLabel) {
			sec_type,
			SectionRelocationLabel::Label::SectionName
		};
	}

	constexpr static auto size_reloc_label(SectionType sec_type) -> Relocation::LabelType {
		return (SectionRelocationLabel) {
			sec_type,
			SectionRelocationLabel::Label::SectionSize
		};
	}

	constexpr static auto offset_reloc_label(SectionType sec_type) -> Relocation::LabelType {
		return (SectionRelocationLabel) {
			sec_type,
			SectionRelocationLabel::Label::SectionOffset
		};
	}

	constexpr static auto name_reloc_width() -> u64 {
		return decltype(std::declval<SectionHeader>().sh_name)::serialized_size();
	}

	constexpr static auto size_reloc_width() -> u64 {
		return decltype(std::declval<SectionHeader>().sh_size)::serialized_size();
	}

	constexpr static auto offset_reloc_width() -> u64 {
		return decltype(std::declval<SectionHeader>().sh_offset)::serialized_size();
	}

public:
	// clang-format off
    constexpr static auto serialized_size() -> u64 {
        SectionHeader header;
		return entries_serialized_size(
				header.sh_name, header.sh_type, header.sh_flags,
				header.sh_addr, header.sh_offset, header.sh_size,
				header.sh_link, header.sh_info, header.sh_addralign,
				header.sh_entsize);
    }
	// clang-format on
};

struct Symbol {
	Entry<u32> st_name{};
	Entry<u8> st_info{};
	Entry<u8> st_other{};
	Entry<u16> st_shndx{};
	Entry<u64> st_value{};
	Entry<u64> st_size{};

public:
	// clang-format off
	constexpr static auto serialized_size() -> u64 {
		Symbol sym;
		return entries_serialized_size(
				sym.st_name, sym.st_info, sym.st_other,
				sym.st_shndx, sym.st_value, sym.st_size);
	}
	// clang-format on
};

struct Section {
	ByteVec data;
};

struct StringTable : public Section {
	std::unordered_map<std::string, u64> offsets;

	auto add_string(std::string_view name) -> u64 {
		u64 offset = data.size();
		Utils::extend(data, ByteVec(name.begin(), name.end()));
		data.push_back(0x00);
		offsets[std::string(name)] = offset;
		return offset;
	}
};

struct Serializer {
	using RelocationVec = std::vector<Relocation>;
	using RelocationFixes = std::vector<RelocationFixInfo>;

	ByteVec buffer;
	RelocationVec relocations;
	RelocationFixes relocation_fixes;

	template <typename T>
	auto operator<<(const Entry<T> &entry) -> Serializer & {
		if (entry.is_value()) {
			serialize(entry.value());
		} else {
			Relocation reloc = entry.relocation();
			if (not reloc.offset.has_value()) {
				reloc.offset = buffer.size();
			}
			relocations.push_back(reloc);
			serialize(T{});
		}
		return *this;
	}

	auto operator<<(const Serializer &other) -> Serializer & {
		Utils::extend(buffer, other.buffer);
		Utils::extend(relocations, other.relocations);
		Utils::extend(relocation_fixes, other.relocation_fixes);
		return *this;
	}

	auto operator<<(const ElfHeader &elf_header) -> Serializer & {
		return 
			*this << elf_header.e_ident
				  << elf_header.e_type
			      << elf_header.e_machine
				  << elf_header.e_version
				  << elf_header.e_entry
				  << elf_header.e_phoff
				  << elf_header.e_shoff
				  << elf_header.e_flags
				  << elf_header.e_ehsize
				  << elf_header.e_phentsize
				  << elf_header.e_phnum
				  << elf_header.e_shentsize
				  << elf_header.e_shnum
				  << elf_header.e_shstrndx;
	}

	auto operator<<(const SectionHeader &section_header) -> Serializer & {
		return
			*this << section_header.sh_name
			      << section_header.sh_type
				  << section_header.sh_flags
				  << section_header.sh_addr
				  << section_header.sh_offset
				  << section_header.sh_size
				  << section_header.sh_link
				  << section_header.sh_info
				  << section_header.sh_addralign
				  << section_header.sh_entsize;
	}

	auto operator<<(const Symbol &sym) -> Serializer & {
		return
			*this << sym.st_name
				  << sym.st_info
				  << sym.st_other
				  << sym.st_shndx
				  << sym.st_value
				  << sym.st_size;
	}

	auto operator<<(const Section &section) -> Serializer & {
		serialize(section.data);
		return *this;
	}

public:

	auto fix_relocation(const RelocationFixInfo &reloc_fix_info) {
		RelocationFixInfo patched_reloc_fix = reloc_fix_info;

		if (not reloc_fix_info.value.has_value()) {
			patched_reloc_fix.value = buffer.size();
		}
		relocation_fixes.push_back(patched_reloc_fix);
	}

	auto dump_to_file() -> void {
		u64 relocations_found = relocations.size();
		u64 relocations_fixed = 0;

		for (const auto &reloc_fix_info : relocation_fixes) {
			for (const auto &reloc : relocations) {
				if (reloc.label != reloc_fix_info.label_to_fix) continue;

				if (not reloc.offset.has_value()) {
					fmt::print("Relocation offset does not have a value\n");
					fmt::print("Aborting...\n");
					std::exit(1);
				}
				
				auto real_value = reloc_fix_info.value.value();
				for (u32 idx = 0; idx < reloc_fix_info.value_size_in_bytes; ++idx) {
					buffer[reloc.offset.value() + idx] = real_value & 0xffu;
					real_value >>= 8;
				}
				relocations_fixed++;
			}
		}
		if (relocations_found != relocations_fixed) {
			fmt::print("Didn't fix all the relocations, Found {} relocations but fixed {} ...\n", relocations_found, relocations_fixed);
			//std::exit(1);
		}
		fmt::print("buffer size = {}\n", buffer.size());
		File::write(buffer.data(), buffer.size(), fs::path("./elf_file_final"));
	}

private:

	template <typename T>
		requires UnsignedInt<T>
	auto serialize(T value) -> Serializer & {
		buffer.reserve(buffer.size() + sizeof(T));
		for (u32 idx = 0; idx < sizeof(T); ++idx) {
			buffer.push_back(value & 0xffu);
			value >>= 8;
		}
		return *this;
	}

	template <typename T, u32 sz>
		requires std::same_as<std::remove_cvref_t<T>, u8>
	auto serialize(T (&arr)[sz]) -> Serializer & {
		Utils::extend(buffer, ByteVec(arr, arr + sz));
		return *this;
	}

	template <typename T, usz size>
		requires std::same_as<std::remove_cvref_t<T>, u8>
	auto serialize(std::array<T, size> arr) -> Serializer & {
		Utils::extend(buffer, ByteVec(arr.begin(), arr.end()));
		return *this;
	}

	auto serialize(const ByteVec &vec) -> Serializer & {
		Utils::extend(buffer, vec);
		return *this;
	}

};

auto main(i32 argc, char *argv[]) -> i32 {
	Serializer ser;
    // clang-format off
    ElfHeader elf_header = {
		.e_ident = {
			std::array<u8, 16>{
				ElfHeader::elf_mag_0,
				ElfHeader::elf_mag_1,
				ElfHeader::elf_mag_2,
				ElfHeader::elf_mag_3,
				ElfHeader::elf_class_64,
				ElfHeader::elf_data_2_lsb,
				ElfHeader::ev_current,
				ElfHeader::elf_os_abi_linux,
			}
		},
		.e_type = ElfHeader::et_rel,
		.e_machine = ElfHeader::em_x86_64,
		.e_version = ElfHeader::ev_current,
		.e_entry = u64{0},
		.e_phoff = u64{0},
		.e_shoff = Relocation::with_label(ElfHeaderRelocationLabel::SectionHeaderTableOffset),
		.e_flags = u32{0},
		.e_ehsize = static_cast<u16>(ElfHeader::serialized_size()),
		.e_phentsize = u16{0},
		.e_phnum = u16{0},
		.e_shentsize = static_cast<u16>(SectionHeader::serialized_size()),
		.e_shnum = Relocation::with_label(ElfHeaderRelocationLabel::NumSectionHeaders),
		.e_shstrndx = Relocation::with_label(ElfHeaderRelocationLabel::SectionHeaderStrTabIdx)
	};
	ser << elf_header;

	ser.fix_relocation({
			.label_to_fix = ElfHeaderRelocationLabel::SectionHeaderTableOffset,
			.value_size_in_bytes = 8
	});


	SectionHeader null_section = {
		.sh_name = Relocation::with_label(SectionHeader::name_reloc_label(SectionType::Null)),
		.sh_type = SectionHeader::sht_null,
		.sh_flags = u64{0},
		.sh_addr = u64{0},
		.sh_offset = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::Null,
				SectionRelocationLabel::Label::SectionOffset
			}
		),
		.sh_size = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::Null,
				SectionRelocationLabel::Label::SectionSize
			}
		),
		.sh_link = u32{0},
		.sh_info = u32{0},
		.sh_addralign = u64{1},
		.sh_entsize = u64{0}
	};

	ser << null_section;

    SectionHeader text = {
		.sh_name = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::Text,
				SectionRelocationLabel::Label::SectionName
			}
		),
		.sh_type = SectionHeader::sht_progbits,
		// AHYA(miloudi): give this proper permissions.
		.sh_flags = u64{0},
		.sh_addr = u64{0},
		.sh_offset = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::Text,
				SectionRelocationLabel::Label::SectionOffset
			}
		),
		.sh_size = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::Text,
				SectionRelocationLabel::Label::SectionSize
			}
		),
		.sh_link = u32{0},
		.sh_info = u32{0},
		.sh_addralign = u64{1},
		.sh_entsize = u64{0}
	};

	ser << text;


    SectionHeader symtab = {
		.sh_name = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::SectionName
			}
		),
		.sh_type = SectionHeader::sht_symtab,
		// AHYA(miloudi): give this proper permissions.
		.sh_flags = u64{0},
		.sh_addr = u64{0},
		.sh_offset = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::SectionOffset
			}
		),
		.sh_size = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::SectionSize
			}
		),
		.sh_link = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::Link
			}
		),
		.sh_info = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::Info
			}
		),
		.sh_addralign = u64{8},
		.sh_entsize = Symbol::serialized_size()
	};

	ser << symtab;

	ser.fix_relocation({
			.value = 3,
			.label_to_fix = (SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::Link
			},
			.value_size_in_bytes = 4
	});

	ser.fix_relocation({
			.value = 1,
			.label_to_fix = (SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::Info
			},
			.value_size_in_bytes = 4
	});

    SectionHeader symtab_strtab = {
		.sh_name = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTabStringTable,
				SectionRelocationLabel::Label::SectionName
			}
		),
		.sh_type = SectionHeader::sht_strtab,
		// AHYA(miloudi): give this proper permissions.
		.sh_flags = u64{0},
		.sh_addr = u64{0},
		.sh_offset = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTabStringTable,
				SectionRelocationLabel::Label::SectionOffset
			}
		),
		.sh_size = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTabStringTable,
				SectionRelocationLabel::Label::SectionSize
			}
		),
		.sh_link = u32{0},
		.sh_info = u32{0},
		.sh_addralign = u64{1},
		.sh_entsize = u64{0}
	};

	ser << symtab_strtab;

    SectionHeader sh_strtab = {
		.sh_name = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SectionHeaderStringTable,
				SectionRelocationLabel::Label::SectionName
			}
		),
		.sh_type = SectionHeader::sht_strtab,
		// AHYA(miloudi): give this proper permissions.
		.sh_flags = u64{0},
		.sh_addr = u64{0},
		.sh_offset = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SectionHeaderStringTable,
				SectionRelocationLabel::Label::SectionOffset
			}
		),
		.sh_size = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SectionHeaderStringTable,
				SectionRelocationLabel::Label::SectionSize
			}
		),
		.sh_link = u32{0},
		.sh_info = u32{0},
		.sh_addralign = u64{1},
		.sh_entsize = u64{0}
	};

	ser << sh_strtab;

	ser.fix_relocation({
			.value = 5,
			.label_to_fix = ElfHeaderRelocationLabel::NumSectionHeaders,
			.value_size_in_bytes = 2
	});
	ser.fix_relocation({
			.value = 4,
			.label_to_fix = ElfHeaderRelocationLabel::SectionHeaderStrTabIdx,
			.value_size_in_bytes = 2
	});

	ser.fix_relocation({
			.value = 0,
			.label_to_fix = SectionHeader::size_reloc_label(SectionType::Null),
			.value_size_in_bytes = 8
	});
	ser.fix_relocation({
			.label_to_fix = SectionHeader::offset_reloc_label(SectionType::Null),
			.value_size_in_bytes = 8
	});

	// How to fix those relocations.
	Section text_section;
	text_section.data = {0x48, 0xc7, 0xc0, 0x7b, 0x00, 0x00, 0x00,0xc3};

	ser.fix_relocation({
			.value = 1,
			.label_to_fix = (SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::TextSectionIdx
			},
			.value_size_in_bytes = 2
	});
	ser.fix_relocation({
			.value = text_section.data.size(),
			.label_to_fix = SectionHeader::size_reloc_label(SectionType::Text),
			.value_size_in_bytes = 8
	});
	fmt::print("offset of text section = {}\n", ser.buffer.size());
	ser.fix_relocation({
			.label_to_fix = SectionHeader::offset_reloc_label(SectionType::Text),
			.value_size_in_bytes = 8
	});

	ser << text_section;

	StringTable sh_strtab_section;
	sh_strtab_section.add_string(".text"s);
	sh_strtab_section.add_string(""s);
	sh_strtab_section.add_string(".shstrtab"s);
	sh_strtab_section.add_string(".symtab"s);
	sh_strtab_section.add_string(".strtab"s);

	ser.fix_relocation({
			.value = sh_strtab_section.offsets[".text"s],
			.label_to_fix = SectionHeader::name_reloc_label(SectionType::Text),
			.value_size_in_bytes = 4
			});
	ser.fix_relocation({
			.value = sh_strtab_section.offsets[""s],
			.label_to_fix = SectionHeader::name_reloc_label(SectionType::Null),
			.value_size_in_bytes = 4
	});
	ser.fix_relocation({
			.value = sh_strtab_section.offsets[".shstrtab"s],
			.label_to_fix = SectionHeader::name_reloc_label(SectionType::SectionHeaderStringTable),
			.value_size_in_bytes = 4
	});
	ser.fix_relocation({
			.value = sh_strtab_section.offsets[".symtab"s],
			.label_to_fix = SectionHeader::name_reloc_label(SectionType::SymTab),
			.value_size_in_bytes = 4
	});
	ser.fix_relocation({
			.value = sh_strtab_section.offsets[".strtab"s],
			.label_to_fix = SectionHeader::name_reloc_label(SectionType::SymTabStringTable),
			.value_size_in_bytes = 4
	});

	ser.fix_relocation({
			.label_to_fix = SectionHeader::offset_reloc_label(SectionType::SectionHeaderStringTable),
			.value_size_in_bytes = 8
	});
	ser.fix_relocation({
			.value = sh_strtab_section.data.size(),
			.label_to_fix = SectionHeader::size_reloc_label(SectionType::SectionHeaderStringTable),
			.value_size_in_bytes = 8
			});

	ser << sh_strtab_section;

	StringTable symtab_strtab_section;

	symtab_strtab_section.add_string(""s);
	symtab_strtab_section.add_string("test_function_1"s);


	ser.fix_relocation({
			.label_to_fix = SectionHeader::offset_reloc_label(SectionType::SymTabStringTable),
			.value_size_in_bytes = 8
			});

	ser.fix_relocation({
			.value = symtab_strtab_section.data.size(),
			.label_to_fix = SectionHeader::size_reloc_label(SectionType::SymTabStringTable),
			.value_size_in_bytes = 8
	});

	ser << symtab_strtab_section; 

	Section symtab_section;
	std::vector<Symbol> syms = {Symbol{}};
	syms.push_back((Symbol) {
		.st_name = static_cast<u32>(symtab_strtab_section.offsets["test_function_1"s]),
		.st_info = static_cast<u8>(ELF64_ST_INFO(STB_GLOBAL, STT_FUNC)),
		.st_shndx = Relocation::with_label(
			(SectionRelocationLabel) {
				SectionType::SymTab,
				SectionRelocationLabel::Label::TextSectionIdx
			}
		),
		.st_value = u64{0},
		.st_size = u64{0}
	});

	auto sz = [&] {
		Serializer aux_ser;
		aux_ser << syms[0] << syms[1];
		return aux_ser.buffer.size();
	}();

	fmt::print("Buffer size = {}\n", ser.buffer.size());
	ser.fix_relocation({
			.label_to_fix = SectionHeader::offset_reloc_label(SectionType::SymTab),
			.value_size_in_bytes = 8
	});

	ser.fix_relocation({
			.value = sz,
			.label_to_fix = SectionHeader::size_reloc_label(SectionType::SymTab),
			.value_size_in_bytes = 8
	});

	ser << syms[0];
	ser << syms[1];
	//ser << syms[0] << syms[1];

	ser.dump_to_file();

    return 0;
    // clang-format on
}
//text_section.relocation_fixes.push_back({
//		.value = text_section.data.size(),
//		.label_to_fix = SectionHeader::size_reloc_label(SectionType::Text),
//		.value_size_in_bytes = SectionHeader::size_reloc_width()
//});
//text_section.relocation_fixes.push_back({
//		.label_to_fix = SectionHeader::offset_reloc_label(SectionType::Text),
//		.value_size_in_bytes = SectionHeader::offset_reloc_width()
//});
//sh_strtab_section.relocation_fixes.push_back({
//		.value = sh_strtab_section.add_string(".text"s),
//		.label_to_fix = SectionHeader::name_reloc_label(SectionType::Text),
//		.value_size_in_bytes = SectionHeader::name_reloc_width()
//});
//sh_strtab_section.relocation_fixes.push_back({
//		.value = sh_strtab_section.add_string(".shstrtab"s),
//		.label_to_fix = SectionHeader::name_reloc_label(SectionType::SectionHeaderStringTable),
//		.value_size_in_bytes = SectionHeader::name_reloc_width()
//});
//sh_strtab_section.relocation_fixes.push_back({
//		.value = sh_strtab_section.add_string(".strtab"s),
//		.label_to_fix = SectionHeader::name_reloc_label(SectionType::SymTabStringTable),
//		.value_size_in_bytes = SectionHeader::name_reloc_width()
//});
//sh_strtab_section.relocation_fixes.push_back({
//		.value = sh_strtab_section.add_string(".symtab"s),
//		.label_to_fix = SectionHeader::name_reloc_label(SectionType::SymTab),
//		.value_size_in_bytes = SectionHeader::name_reloc_width()
//});
//symtab_section.relocation_fixes.push_back({
//		.value = symtab_section.data.size(),
//		.label_to_fix = SectionHeader::size_reloc_label(SectionType::SymTab),
//		.value_size_in_bytes = SectionHeader::size_reloc_width()
//});
//symtab_section.relocation_fixes.push_back({
//		.label_to_fix = SectionHeader::offset_reloc_label(SectionType::SymTab),
//		.value_size_in_bytes = SectionHeader::offset_reloc_width()
//});
