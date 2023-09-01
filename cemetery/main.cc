#include <exception>
#include <span>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "assembler/elf.hh"
#include "base.hh"
#include "fmt/format.h"

//using ByteVec = std::vector<u8>;
//
//struct Section {
//    std::string name{};
//    SectionHeader header{};
//    ByteVec body{};
//
//    struct Relocation {
//        enum struct RelocationLabel {
//            HeaderName,
//            SectionFileOffset,
//            SectionSize,
//			Invalid
//        };
//        RelocationLabel label = RelocationLabel::Invalid;
//        u64 offset;
//        std::optional<ByteVec> fix;
//    };
//    std::vector<Relocation> relocations{};
//
//    enum struct Type {
//        SectionHeadersStringTable,
//        Null,
//        Text,
//		Invalid
//    };
//    Type type = Type::Invalid;
//};
//
//// =========================================================
//// New implementation
//// =========================================================
//struct SectionHeader2 {
//    u32 sh_name{}; // depends on the string table being written
//    u32 sh_type{};
//    u64 sh_flags{};
//    u64 sh_addr{};
//    u64 sh_offset{}; // offset into the file. Depends on the entire section being written.
//    u64 sh_size{};
//    u32 sh_link{};
//    u32 sh_info{};
//    u64 sh_addralign{};
//    u64 sh_entsize{};
//};
//
//struct SectionHeadersStringTable2 {
//    std::string name = ".shstrtab\0"s;
//    SectionHeader2 header{};
//    std::vector<u8> body{};
//
//    struct Relocation {
//        enum struct RelocationName {
//            Name,
//            OffsetIntoFile,
//            SectionSize
//        };
//
//        RelocationName reloc_name;
//        u64 offset{};
//    };
//};

using fiska::elf::ElfHeader;
using fiska::elf::SectionHeader;

constexpr char kStringTableSectionName[] = ".shstrtab";
constexpr char kTextSectionName[] = ".text";

template <typename Enum>
    requires std::is_enum_v<Enum>
constexpr auto operator+(Enum e) -> std::underlying_type_t<Enum> {
    return std::to_underlying(e);
}

template <typename... T, typename... U>
auto is(const std::variant<U...> &v) -> bool {
    return (... or std::holds_alternative<T>(v));
}

struct SectionHeadersStringTable {};
struct NullSectionHeader {};
struct Text {};

// clang-format off
using SectionHeaderTable = std::vector<
	std::variant<
		SectionHeader<SectionHeadersStringTable>,
		SectionHeader<Text>,
		SectionHeader<NullSectionHeader>
	>
>;
// clang-format on

template <typename T>
struct Section {
    using section_type = T;
};

template <>
struct Section<SectionHeadersStringTable> {
    std::vector<u8> bytes = {0x00};

    auto add_string(std::string_view section_name) -> u64 {
        u64 offset = bytes.size();
        extend(bytes, std::vector<u8>(section_name.begin(), section_name.end()));
        bytes.push_back(0x00);
        return offset;
    }
};

template <>
struct Section<Text> {
    std::vector<u8> bytes = {0x48, 0xc7, 0xc0, 0x7b, 0x00, 0x00, 0x00, 0xc3};
};

enum struct RelocationName : u32 {
    SectionHeaderTableOffset,
    SectionHeaderStringTableIndex,
    NumberOfSectionHeaders,
    SectionHeadersStringTableName,
    SectionHeadersStringTableOffset,
    SectionHeadersStringTableSize,
    TextSectionHeaderName,
    TextSectionOffset,
    TextSectionSize,
    Invalid,
};

// Relocations are just offsets into the file.
struct Relocation {
    u64 offset{};
    RelocationName relname = RelocationName::Invalid;

    Relocation(u64 _offset, RelocationName _relname) : offset(_offset), relname(_relname) {}
    Relocation() {}
};

// TODO(miloudi): This is kind of a misnomer. Change this name to something else.
struct RelocationHandler {
    std::unordered_map<u32, Relocation> relocations;
    std::unordered_map<u32, std::vector<u8>> relocation_fixes;
    std::vector<u8> &bytes;

    RelocationHandler(std::vector<u8> &bytes_) : bytes(bytes_) {}

    auto add_relocation(Relocation rel) -> void { relocations.insert({+rel.relname, rel}); }

    template <typename T>
    auto fix_relocation(std::vector<u8> &bytes, RelocationName relname, T value) -> void {
        relocation_fixes.insert({+relname, serialize(value)});
    }

    auto fix_relocations() -> void {
        if (relocation_fixes.size() != relocations.size()) {
            fmt::print(
                "The number of relocations is not the same as the number of relocation fixes\n");
            fmt::print("Skipping this step...\n");
        } else {
            for (const auto &[reloc_id, data] : relocation_fixes) {
                u64 offset = relocations[reloc_id].offset;

                for (u32 i = 0; i < data.size(); ++i) {
                    bytes[offset + i] = data[i];
                }
            }
        }
    }
};

// Only one instance of this should be around.
struct Serializer {
    std::vector<u8> bytes;
    RelocationHandler relocation_handler{bytes};

    auto operator<<(std::span<u8> data) -> Serializer &;
    auto operator<<(const std::vector<u8> &data) -> Serializer &;

    auto operator<<(const ElfHeader &elf_header) -> Serializer &;
    auto operator<<(const SectionHeader<Text> &text_section_header) -> Serializer &;
    auto operator<<(const Section<Text> &text_section) -> Serializer &;
    auto operator<<(const SectionHeader<SectionHeadersStringTable> &section_headers_strtab)
        -> Serializer &;
    auto operator<<(const SectionHeader<NullSectionHeader> &section_headers_strtab) -> Serializer &;
    auto operator<<(Section<SectionHeadersStringTable> &section_headers_strtab) -> Serializer &;
    auto operator<<(const SectionHeaderTable &section_header_table) -> Serializer &;

    auto offset() -> u64 { return bytes.size(); }

    auto dump_to_file(const fs::path &path) -> void;
};

auto Serializer::dump_to_file(const fs::path &path) -> void {
    relocation_handler.fix_relocations();
    File::write(bytes.data(), bytes.size(), path);
}

auto Serializer::operator<<(const std::vector<u8> &data) -> Serializer & {
    std::copy(data.begin(), data.end(), std::back_inserter(bytes));
    return *this;
}

auto Serializer::operator<<(std::span<u8> data) -> Serializer & {
    std::copy(data.begin(), data.end(), std::back_inserter(bytes));
    return *this;
}

auto Serializer::operator<<(const ElfHeader &elf_header) -> Serializer & {
    // clang-format off
	*this << serialize(elf_header.e_ident)
		  << serialize(elf_header.e_type)
		  << serialize(elf_header.e_machine)
		  << serialize(elf_header.e_version)
		  << serialize(elf_header.e_entry)
		  << serialize(elf_header.e_phoff);
    // clang-format on

    relocation_handler.add_relocation(
        Relocation(offset(), RelocationName::SectionHeaderTableOffset));
    *this << serialize(elf_header.e_shoff);

    // clang-format off
	*this << serialize(elf_header.e_flags)
		  << serialize(elf_header.e_ehsize)
		  << serialize(elf_header.e_phentsize)
		  << serialize(elf_header.e_phnum)
		  << serialize(elf_header.e_shentsize);
    // clang-format on

    relocation_handler.add_relocation(Relocation(offset(), RelocationName::NumberOfSectionHeaders));
    *this << serialize(elf_header.e_shnum);

    relocation_handler.add_relocation(
        Relocation(offset(), RelocationName::SectionHeaderStringTableIndex));
    return *this << serialize(elf_header.e_shstrndx);
}

auto Serializer::operator<<(const SectionHeader<SectionHeadersStringTable> &section_headers_strtab)
    -> Serializer & {
    relocation_handler.add_relocation(
        Relocation(offset(), RelocationName::SectionHeadersStringTableName));
    *this << serialize(section_headers_strtab.sh_name);

    // clang-format off
	*this << serialize(section_headers_strtab.sh_type)
		  << serialize(section_headers_strtab.sh_flags)
		  << serialize(section_headers_strtab.sh_addr);
    // clang-format on

    relocation_handler.add_relocation(
        Relocation(offset(), RelocationName::SectionHeadersStringTableOffset));
    *this << serialize(section_headers_strtab.sh_offset);

    relocation_handler.add_relocation(
        Relocation(offset(), RelocationName::SectionHeadersStringTableSize));
    *this << serialize(section_headers_strtab.sh_size);

    // clang-format off
	return *this << serialize(section_headers_strtab.sh_link)
				 << serialize(section_headers_strtab.sh_info)
				 << serialize(section_headers_strtab.sh_addralign)
				 << serialize(section_headers_strtab.sh_entsize);
    // clang-format on
}

auto Serializer::operator<<(Section<SectionHeadersStringTable> &section_headers_strtab)
    -> Serializer & {
    u64 name_offset = section_headers_strtab.add_string(kStringTableSectionName);
    u64 size = section_headers_strtab.bytes.size();

    relocation_handler.fix_relocation(bytes, RelocationName::SectionHeadersStringTableName,
                                      static_cast<u32>(name_offset));
    relocation_handler.fix_relocation(bytes, RelocationName::SectionHeadersStringTableSize,
                                      static_cast<u64>(size));
    relocation_handler.fix_relocation(bytes, RelocationName::SectionHeadersStringTableOffset,
                                      static_cast<u64>(offset()));

    return *this << section_headers_strtab.bytes;
}

auto Serializer::operator<<(const Section<Text> &text_section) -> Serializer & {
    u64 size = text_section.bytes.size();

    relocation_handler.fix_relocation(bytes, RelocationName::TextSectionSize,
                                      static_cast<u64>(size));
    relocation_handler.fix_relocation(bytes, RelocationName::TextSectionOffset,
                                      static_cast<u64>(offset()));

    return *this << text_section.bytes;
}

auto Serializer::operator<<(const SectionHeader<Text> &text_section_header) -> Serializer & {
    *this << serialize(text_section_header.sh_name);

    // clang-format off
	*this << serialize(text_section_header.sh_type)
		  << serialize(text_section_header.sh_flags)
		  << serialize(text_section_header.sh_addr);
    // clang-format on

    relocation_handler.add_relocation(Relocation(offset(), RelocationName::TextSectionOffset));
    *this << serialize(text_section_header.sh_offset);

    relocation_handler.add_relocation(Relocation(offset(), RelocationName::TextSectionSize));
    *this << serialize(text_section_header.sh_size);

    // clang-format off
	return *this << serialize(text_section_header.sh_link)
				 << serialize(text_section_header.sh_info)
				 << serialize(text_section_header.sh_addralign)
				 << serialize(text_section_header.sh_entsize);
}

auto Serializer::operator<<(const SectionHeader<NullSectionHeader> &null_section_header)
    -> Serializer & {
    *this << serialize(null_section_header.sh_name);

    // clang-format off
	*this << serialize(null_section_header.sh_type)
		  << serialize(null_section_header.sh_flags)
		  << serialize(null_section_header.sh_addr);
    // clang-format on

    *this << serialize(null_section_header.sh_offset);

    *this << serialize(null_section_header.sh_size);

    // clang-format off
	return *this << serialize(null_section_header.sh_link)
				 << serialize(null_section_header.sh_info)
				 << serialize(null_section_header.sh_addralign)
				 << serialize(null_section_header.sh_entsize);
    // clang-format on
}

auto Serializer::operator<<(const SectionHeaderTable &section_header_table) -> Serializer & {
    relocation_handler.fix_relocation(bytes, RelocationName::SectionHeaderTableOffset,
                                      static_cast<u64>(offset()));
    relocation_handler.fix_relocation(bytes, RelocationName::NumberOfSectionHeaders,
                                      static_cast<u16>(section_header_table.size()));

    for (u32 idx = 0; idx < section_header_table.size(); ++idx) {
        const auto &section_header = section_header_table[idx];

        if (is<SectionHeader<SectionHeadersStringTable>>(section_header)) {
            const auto &section_header_strtab =
                std::get<SectionHeader<SectionHeadersStringTable>>(section_header);
            relocation_handler.fix_relocation(bytes, RelocationName::SectionHeaderStringTableIndex,
                                              static_cast<u16>(idx));

            *this << section_header_strtab;
        } else if (is<SectionHeader<Text>>(section_header)) {
            *this << std::get<SectionHeader<Text>>(section_header);
        } else if (is<SectionHeader<NullSectionHeader>>(section_header)) {
            *this << std::get<SectionHeader<NullSectionHeader>>(section_header);
        } else {
            fmt::print("Forgot to handle more section headers\n");
            std::terminate();
        }
    }
    return *this;
}

auto main(int argc, char *argv[]) -> int {
    fmt::print("Bismillah\n");
    Serializer serializer;

    ElfHeader elf_header;
    Section<SectionHeadersStringTable> section_headers_strtab;

    SectionHeader<NullSectionHeader> null_section_header;
    null_section_header.sh_type = SHT_NULL;

    SectionHeader<SectionHeadersStringTable> section_headers_strtab_header;
    section_headers_strtab_header.sh_type = SHT_STRTAB;
    section_headers_strtab_header.sh_addralign = 1;

    SectionHeader<Text> text_section_header;
    text_section_header.sh_name =
        static_cast<u32>(section_headers_strtab.add_string(kTextSectionName));
    text_section_header.sh_type = SHT_PROGBITS;
    text_section_header.sh_addralign = 1;

    SectionHeaderTable section_header_table;
    section_header_table.push_back(null_section_header);
    section_header_table.push_back(text_section_header);
    section_header_table.push_back(section_headers_strtab_header);

    serializer << elf_header;
    serializer << section_header_table;
    serializer << section_headers_strtab;
    serializer << Section<Text>{};

    fmt::print("serializer file size = {}\n", serializer.bytes.size());
    serializer.dump_to_file("./test_elf_file");
    // The order is as follows:
    // 1- first create the string_table and the text.
    // 2- create the section headers corresponding for both of them.
    // 3- keep track of all the relocations you need to do.
    // 4- create the elf header.
    // 5- write everything to a buffer starting from the elf header -> sections -> section
    // header. 6- Fix all the relocations required.
    return 0;
}
//struct Section {
//    std::string name{};
//    SectionHeader header{};
//    ByteVec body{};
//
//    enum struct Type {
//        SectionHeadersStringTable,
//        Null,
//        Text,
//        Invalid
//    };
//    Type type = Type::Invalid;
//
//    struct Relocation {
//        enum struct Label {
//            HeaderName,
//            SectionFileOffset,
//			SectionSize,
//            Invalid
//        };
//        Label label = Label::Invalid;
//		Type section_type = Section::Type::Invalid;
//        u64 offset;
//    };
//    std::vector<Relocation> relocations{};
//
//public:
//	auto add_relocation(Relocation reloc) -> void {
//		relocations.push_back(reloc);
//	}
//};
//
//void add_relocation(const Section &section, Relocation reloc) {
//	section.relocations.push_back(reloc);
//}
//
//struct ByteSerializer {
//	ByteVec buffer;
//
//	template <typename T>
//	auto operator<<(const T &value) -> ByteSerializer & {
//		::detail::extend(buffer, serialize(value));
//		return *this;
//	}
//
//	auto offset() -> u64 {
//		return buffer.size();
//	}
//
//private:
//	template <typename T>
//		requires ::detail::is_any_of<std::remove_cvref_t<T>, u8, u16, u32, u64>
//	auto serialize(T value) -> ByteVec {
//		ByteVec ret;
//		ret.reserve(sizeof value);
//
//		for (u32 i = 0; i < sizeof value; ++i) {
//			ret.push_back(value & 0xffu);
//			value >>= 8;
//		}
//		return ret;
//	}
//
//	template <typename T, u32 sz>
//		requires std::same_as<std::remove_cvref_t<T>, u8>
//	auto serialize(T (&arr)[sz]) -> ByteVec {
//		return std::vector<u8>(arr, arr + sz);
//	}
//
//	auto serialize(const ByteVec &v) -> ByteVec {
//		return v;
//	}
//
//	auto serialize(std::span<Section> section_header_table) -> ByteVec {
//		ByteSerializer serializer;
//
//		ByteVec section_headers_strtab;
//		for (u32 idx = 0; idx < section_header_table.size(); ++idx) {
//			const Section &section = section_header_table[idx];
//			::detail::extend(section_headers_strtab, std::vector<u8>(section.name.begin(), section.name.end()));
//		}
//
//		// clang-format off
//		for (u32 idx = 0; idx < section_header_table.size(); ++idx) {
//			const Section &section = section_header_table[idx];
//			auto relocation = [&](Section::Relocation::Label label) {
//				return (Section::Relocation) {
//					.offset = offset(),
//					.label = label,
//					.type = section.type
//				};
//			};
//
//			serializer << (section.add_relocation(relocation(Section::Relocation::Label::HeaderName)), section.header.sh_name)
//				       << section.header.sh_type
//					   << section.header.sh_flags
//					   << section.header.sh_addr
//					   << (section.add_relocation(relocation(Section::Relocation::Label::SectionFileOffset)), section.header.sh_offset)
//					   << (section.add_relocation(relocation(Section::Relocation::Label::SectionSize)), section.header.sh_size)
//					   << section.header.sh_link
//					   << section.header.sh_info
//					   << section.header.sh_addralign
//					   << section.header.entsize;
//		}
//		// clang-format on
//
//		for (u32 idx = 0; idx < section_header_table.size(); ++idx) {
//			const Section &section = section_header_table[idx];
//		}
//	}
//};
//
//struct Serializer {
//    ByteVec out;
//
//	template <typename T>
//	auto relocate(const T &value, const Section &section, Section::Relocation::Label label) -> T {
//		Relocation reloc = {
//			.offset = offset(),
//			.type = section.type,
//			.label = label
//		};
//		section.relocations.push_back(reloc);
//		return value;
//	}
//
//	template <typename T>
//	auto fix_relocation(const Section &section, Section::RelocationLabel label, const T &value) -> Serializer & {
//		for (const auto &reloc : section.relocations) {
//			if (reloc.label != label) {
//				continue;
//			}
//
//			ByteVec ser_value = ::detail::serialize(value);
//			for (u32 i = 0; i < ser_value.size(); ++i) {
//				out[reloc.offset + i] = ser_value[i];
//			}
//		}
//	}
//		
//
//    template <typename T>
//    auto operator<<(const T &value) -> Serializer & {
//		// forward here.
//        ByteVec serialized = ::detail::serialize(value);
//        std::copy(serialized.begin(), serialized.end(), std::back_inserter(out));
//        return *this;
//    }
//
//	auto operator<<(const std::vector<Section> &section_table) -> Serializer & {
//		// create teh body of the section_headers_strtab from the section tabel.
//		// iterate over all the names and create your body.
//		for (u32 idx = 0; idx < section_table.size(); ++idx) {
//			const auto &section = section_table[idx];
//
//			//clang-format off
//			*this << relocate(section.header.sh_name, section, Section::RelocationLabel::HeaderName)
//				  << section.header.sh_type
//				  << section.header.sh_flags
//				  << section.header.sh_addr
//				  << relocate(section.header.sh_offset, section, Section::Relocation::Label::SectionFileOffset)
//				  << section.header.sh_size
//				  << section.header.sh_link
//				  << section.header.sh_info
//				  << section.header.sh_addralign
//				  << section.header.entsize;
//			//clang-format on
//		}
//
//		for (u32 idx = 0; idx < section_table.size(); ++idx) {
//			const auto &section = section_table[idx];
//
//			*this << fix_relocation(section, Section::Relocation::Label::SectionFileOffset, (u32) out.size())
//				  << section.body;
//
//		}
//
//		// write the section headers string table as the last step.
//
//		return *this;
//	}
//};
