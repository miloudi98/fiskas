#ifndef __FISKA_ASSEMBLER_ELF_ELF_TYPES_HH__
#define __FISKA_ASSEMBLER_ELF_ELF_TYPES_HH__

#include <vector>
#include <unordered_map>
#include <numeric>

#include "base.hh"

struct StringTable {
	std::vector<u8> out;

	auto add_string(std::string_view name) -> u64 {
		u64 offset = out.size();

		::detail::extend(out, std::vector<u8>(name.begin(), name.end()));
		out.push_back(0x00);

		return offset;
	}
};

enum struct SectionType : u16 {
	Null = 0,
	Text = 1,
	SectionHeaderStrTab = 2,
	SymTab = 3,
	SymTabStrTab = 4,
	Data = 5,
};

struct SectionTypeHash {
	[[nodiscard]] bool operator()(SectionType sec_ty) const { return +sec_ty; }
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

public:
    constexpr static u8 sht_null = 0;
    constexpr static u8 sht_progbits = 1;
    constexpr static u8 sht_symtab = 2;
    constexpr static u8 sht_strtab = 3;

public:
	constexpr static auto serialized_size() -> u64 {
		SectionHeader header;
		return sizeof header.sh_name
			 + sizeof header.sh_type
			 + sizeof header.sh_flags
			 + sizeof header.sh_addr
			 + sizeof header.sh_offset
			 + sizeof header.sh_size
			 + sizeof header.sh_link
			 + sizeof header.sh_info
			 + sizeof header.sh_addralign
			 + sizeof header.sh_entsize;
	}
};

struct Symbol {
	u32 st_name{};
	u8 st_info{};
	u8 st_other{};
	u16 st_shndx{};
	u64 st_value{};
	u64 st_size{};

public:
	constexpr static auto serialized_size() -> u64 {
		Symbol sym;
		return sizeof sym.st_name
			 + sizeof sym.st_info
			 + sizeof sym.st_other
			 + sizeof sym.st_shndx
			 + sizeof sym.st_value
			 + sizeof sym.st_size;
	}
};

struct SectionTable {
	using SectionBody = std::vector<u8>;
	using HeaderSectionPair = std::tuple<SectionHeader, SectionBody>;
	using Sections = std::unordered_map<SectionType, HeaderSectionPair, SectionTypeHash, std::equal_to<>>;
	using SectionMapEntry = std::pair<SectionType, HeaderSectionPair>;

	Sections sections = {
		{SectionType::Null, {SectionHeader{}, std::vector<u8>{}}},
		{SectionType::Text, {SectionHeader{}, std::vector<u8>{}}},
		{SectionType::SectionHeaderStrTab, {SectionHeader{}, std::vector<u8>{}}},
		{SectionType::SymTab, {SectionHeader{}, std::vector<u8>{}}},
		{SectionType::Data, {SectionHeader{}, std::vector<u8>{}}},
		{SectionType::SymTabStrTab, {SectionHeader{}, std::vector<u8>{}}}
	};

public:
	constexpr static auto hdr_idx = 0;
	constexpr static auto sec_body_idx = 1;
	const static std::unordered_map<SectionType, const char *> section_names;

public:
	SectionTable() {
		build_sh_strtab_and_fix_all_hdr_name_offsets();
	}

	auto header(SectionType sec_ty) -> SectionHeader & {
		return std::get<hdr_idx>(sections[sec_ty]);
	}

	auto body(SectionType sec_ty) -> SectionBody & {
		return std::get<sec_body_idx>(sections[sec_ty]);
	}

	auto size_of_all_section_bodies() -> u64 {
		return 
			std::accumulate(sections.begin(), sections.end(), u64{0},
				[](u64 size, const SectionMapEntry &sec_map_entry) {
					return size + std::get<sec_body_idx>(sec_map_entry.second).size();
				}
			);
	}

	auto build_sh_strtab_and_fix_all_hdr_name_offsets() -> void {
		body(SectionType::SectionHeaderStrTab) = [&] {
			StringTable sh_strtab;
			for (const auto [sec_ty, name] : section_names) {
				header(sec_ty).sh_name = static_cast<u32>(sh_strtab.add_string(name));
			}
			return sh_strtab.out;
		}();

		SectionHeader &sh_strtab_header = header(SectionType::SectionHeaderStrTab);
		sh_strtab_header.sh_type = SectionHeader::sht_strtab;
		sh_strtab_header.sh_size = body(SectionType::SectionHeaderStrTab).size();
		sh_strtab_header.sh_addralign = 1;
	}

public:
	constexpr static auto sec_ty_from_idx(u16 idx) -> SectionType {
		switch(idx) {
			case 0:
				return SectionType::Null;
			case 1:
				return SectionType::Text;
			case 2:
				return SectionType::SectionHeaderStrTab;
			case 3:
				return SectionType::SymTab;
			case 4:
				return SectionType::SymTabStrTab;
			case 5:
				return SectionType::Data;
			default:
				fmt::print("Idx does not refer to a valid section...\n");
				__builtin_unreachable();

		}
	}

public:
	constexpr static auto headers_size() -> u64 {
		return num_sections() * SectionHeader::serialized_size();
	}

	constexpr static auto num_sections() -> u64 {
		SectionTable sectable;
		return sectable.sections.size();
	}
};

struct ElfHeader {
    std::array<u8, 16> e_ident{};
    u16 e_type{};
    u16 e_machine{};
    u32 e_version{};
    u64 e_entry{};
    u64 e_phoff{};
    u64 e_shoff{};
    u32 e_flags{};
    u16 e_ehsize{};
    u16 e_phentsize{};
    u16 e_phnum{};
    u16 e_shentsize{};
    u16 e_shnum{};
    u16 e_shstrndx{};

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
		return header.e_ident.size()
			 + sizeof header.e_type
			 + sizeof header.e_machine
			 + sizeof header.e_version
			 + sizeof header.e_entry
			 + sizeof header.e_phoff
			 + sizeof header.e_shoff
			 + sizeof header.e_flags
			 + sizeof header.e_ehsize
			 + sizeof header.e_phentsize
			 + sizeof header.e_phnum
			 + sizeof header.e_shentsize
			 + sizeof header.e_shnum
			 + sizeof header.e_shstrndx;
	}

	constexpr static auto create_with_default_params() -> ElfHeader {
		return {
			.e_ident = {
				ElfHeader::elf_mag_0,
				ElfHeader::elf_mag_1,
				ElfHeader::elf_mag_2,
				ElfHeader::elf_mag_3,
				ElfHeader::elf_class_64,
				ElfHeader::elf_data_2_lsb,
				ElfHeader::ev_current,
				ElfHeader::elf_os_abi_linux,
			},
			.e_type = ElfHeader::et_rel,
			.e_machine = ElfHeader::em_x86_64,
			.e_version = ElfHeader::ev_current,
			.e_shoff = ElfHeader::serialized_size(),
			.e_ehsize = static_cast<u16>(ElfHeader::serialized_size()),
			.e_shentsize = static_cast<u16>(SectionHeader::serialized_size()),
			.e_shnum = static_cast<u16>(SectionTable::num_sections()),
			.e_shstrndx = +SectionType::SectionHeaderStrTab
		};
	}
};

#endif  // __FISKA_ASSEMBLER_ELF_ELF_TYPES_HH__
