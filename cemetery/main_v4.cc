#include <array>
#include <utility>
#include <numeric>
#include <concepts>
#include <type_traits>
#include <unordered_map>

#include "base.hh"
#include "fmt/format.h"

using ByteVec = std::vector<u8>;

struct StringTable {
	ByteVec out;

	auto add_string(std::string_view name) -> u64 {
		u64 offset = out.size();

		::detail::extend(out, ByteVec(name.begin(), name.end()));
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

// Remove these ugly macros.
#define ELF64_ST_INFO(bind, type) (((bind)<<4)+((type)&0xf))
#define STB_GLOBAL 1
#define STV_DEFAULT
#define STT_FUNC 2
#define STT_OBJECT 1

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
	using SectionBody = ByteVec;
	using HeaderSectionPair = std::tuple<SectionHeader, SectionBody>;
	using Sections = std::unordered_map<SectionType, HeaderSectionPair, SectionTypeHash, std::equal_to<>>;
	using SectionMapEntry = std::pair<SectionType, HeaderSectionPair>;

	Sections sections = {
		{SectionType::Null, {SectionHeader{}, ByteVec{}}},
		{SectionType::Text, {SectionHeader{}, ByteVec{}}},
		{SectionType::SectionHeaderStrTab, {SectionHeader{}, ByteVec{}}},
		{SectionType::SymTab, {SectionHeader{}, ByteVec{}}},
		{SectionType::Data, {SectionHeader{}, ByteVec{}}},
		{SectionType::SymTabStrTab, {SectionHeader{}, ByteVec{}}}
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

const std::unordered_map<SectionType, const char *> SectionTable::section_names = {
		{SectionType::Null, ""},
		{SectionType::Text, ".text"},
		{SectionType::SectionHeaderStrTab, ".shstrtab"},
		{SectionType::SymTab, ".symtab"},
		{SectionType::Data, ".data"},
		{SectionType::SymTabStrTab, ".strtab"}
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

struct Serializer {
	ByteVec out;

	template <::detail::UnsignedInt T>
	auto serialize(T value) -> Serializer & {
		out.reserve(out.size() + sizeof (value));

		for (u32 idx = 0; idx < sizeof (value); ++idx) {
			out.push_back(value & 0xffu);
			value >>= 8;
		}
		return *this;
	}

	auto serialize(const Symbol &sym) -> Serializer & {
		return serialize(sym.st_name)
			.serialize(sym.st_info)
			.serialize(sym.st_other)
			.serialize(sym.st_shndx)
			.serialize(sym.st_value)
			.serialize(sym.st_size);
	}

	template <typename T>
	auto serialize(const std::vector<T> &data) -> Serializer & {
		for (const T& elem : data) {
			serialize(elem);
		}
		return *this;
	}

	template <usz size>
	auto serialize(const std::array<u8, size> &arr) -> Serializer & {
		for (u32 idx = 0; idx < size; ++idx) {
			serialize(arr[idx]);
		}
		return *this;
	}

	auto serialize(const ElfHeader &elf_header) -> Serializer & {
		return serialize(elf_header.e_ident)
			  .serialize(elf_header.e_type)
			  .serialize(elf_header.e_machine)
			  .serialize(elf_header.e_version)
			  .serialize(elf_header.e_entry)
			  .serialize(elf_header.e_phoff)
			  .serialize(elf_header.e_shoff)
			  .serialize(elf_header.e_flags)
			  .serialize(elf_header.e_ehsize)
			  .serialize(elf_header.e_phentsize)
			  .serialize(elf_header.e_phnum)
			  .serialize(elf_header.e_shentsize)
			  .serialize(elf_header.e_shnum)
			  .serialize(elf_header.e_shstrndx);
	}

	auto serialize(const SectionHeader &header) -> Serializer & {
		return serialize(header.sh_name)
			  .serialize(header.sh_type)
			  .serialize(header.sh_flags)
			  .serialize(header.sh_addr)
			  .serialize(header.sh_offset)
			  .serialize(header.sh_size)
			  .serialize(header.sh_link)
			  .serialize(header.sh_info)
			  .serialize(header.sh_addralign)
			  .serialize(header.sh_entsize);
	}

	auto serialize(const ByteVec &data) -> Serializer & {
		::detail::extend(out, data);
		return *this;
	}
};

struct Code {
	ByteVec text;
	ByteVec data;

	struct Symbol {
		u64 offset{};
		SectionType code_section;
		std::string name{};
		u64 value{};
	};
	std::vector<Symbol> symbols;

public:
	static auto create_dummy_code() -> Code {
		Code c;
		c.text = {0x48, 0xc7, 0xc0, 0x8b, 0x01, 0x00, 0x00,0xc3, 0x48, 0xc7, 0xc0, 0x8d, 0x01, 0x00, 0x00,0xc3};
		c.data = {0xff, 0xff, 0xff, 0x7f};

		c.symbols.push_back({
				.offset = 0,
				.code_section = SectionType::Text,
				.name = "test_function_1"s,
		});
		c.symbols.push_back({
				.offset = 8,
				.code_section = SectionType::Text,
				.name = "test_function_2"s,
		});
		c.symbols.push_back({
				.offset = 0,
				.code_section = SectionType::Data,
				.name = "global_variable"s,
				.value = 4
		});
		return c;
	}
};


auto extract_syms_and_sym_strtab(const Code &code) -> std::pair<std::vector<Symbol>, StringTable> {
	StringTable sym_strtab;
	std::vector<Symbol> symbols;

	// Add dummy symbol
	symbols.push_back(Symbol{});
	sym_strtab.add_string("");

	for (const auto &code_sym : code.symbols) {
		u8 code_sym_info = code_sym.code_section == SectionType::Text 
			? ELF64_ST_INFO(STB_GLOBAL, STT_FUNC) 
			: ELF64_ST_INFO(STB_GLOBAL, STT_OBJECT);

		symbols.push_back(
			{
				.st_name = static_cast<u32>(sym_strtab.add_string(code_sym.name)),
				.st_info = code_sym_info,
				.st_shndx = +code_sym.code_section,
				.st_value = code_sym.offset,
				.st_size = code_sym.value
			}
		);
	}

	return { symbols, sym_strtab };
}

auto build_all_sections(SectionTable &sec_tab, const Code &code) -> void {
	auto [elf_syms, sym_strtab] = extract_syms_and_sym_strtab(code);

	sec_tab.body(SectionType::Text) = code.text;
	sec_tab.body(SectionType::Data) = code.data;
	sec_tab.body(SectionType::SymTabStrTab) = sym_strtab.out;
	// Stack overflow link about the lifetime of temporaries.
	// In this case the Serializer() temp will live until the end of the 
	// expression.
	// https://stackoverflow.com/questions/584824/guaranteed-lifetime-of-temporary-in-c
	sec_tab.body(SectionType::SymTab) = Serializer().serialize(elf_syms).out; 

	SectionHeader &text_header = sec_tab.header(SectionType::Text);
	text_header.sh_type = SectionHeader::sht_progbits;
	text_header.sh_size = sec_tab.body(SectionType::Text).size();
	text_header.sh_addralign = 1;

	SectionHeader &data_header = sec_tab.header(SectionType::Data);
	data_header.sh_type = SectionHeader::sht_progbits;
	data_header.sh_size = sec_tab.body(SectionType::Data).size();
	data_header.sh_addralign = 8;

	SectionHeader &symtab_strtab_header = sec_tab.header(SectionType::SymTabStrTab);
	symtab_strtab_header.sh_type = SectionHeader::sht_strtab;
	symtab_strtab_header.sh_size = sec_tab.body(SectionType::SymTabStrTab).size();
	symtab_strtab_header.sh_addralign = 1;

	SectionHeader &symtab_header = sec_tab.header(SectionType::SymTab);
	symtab_header.sh_type = SectionHeader::sht_symtab;
	symtab_header.sh_size = sec_tab.body(SectionType::SymTab).size();
	symtab_header.sh_link = +SectionType::SymTabStrTab;
	symtab_header.sh_info = 1;
	symtab_header.sh_addralign = 1;
	symtab_header.sh_entsize = Symbol::serialized_size();
}

auto build_elf_file(const Code &code) -> void {
	SectionTable sec_tab;
	Serializer ser;

	ElfHeader elf_header = ElfHeader::create_with_default_params();
	build_all_sections(sec_tab, code);

	elf_header.e_shoff = ElfHeader::serialized_size() + sec_tab.size_of_all_section_bodies();
	ser.serialize(elf_header);

	for (u16 sec_idx = 0; sec_idx < SectionTable::num_sections(); ++sec_idx) {
		SectionType sec_ty = SectionTable::sec_ty_from_idx(sec_idx);

		sec_tab.header(sec_ty).sh_offset = ser.out.size();
		ser.serialize(sec_tab.body(sec_ty));
	}

	for (u16 sec_idx = 0; sec_idx < SectionTable::num_sections(); ++sec_idx) {
		SectionType sec_ty = SectionTable::sec_ty_from_idx(sec_idx);
		ser.serialize(sec_tab.header(sec_ty));
	}

	File::write(ser.out.data(), ser.out.size(), fs::path("./elf_file_v2"));
}

auto main(i32 argc, char *argv[]) -> i32 {
    fmt::print("Bismillah\n");
	Code code = Code::create_dummy_code();
	build_elf_file(code);
    return 0;
}
