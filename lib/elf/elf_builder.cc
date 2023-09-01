#include "elf/elf_builder.hh"
#include "elf/serializer.hh"

#define ELF64_ST_INFO(bind, type) (((bind)<<4)+((type)&0xf))
#define STB_GLOBAL 1
#define STV_DEFAULT
#define STT_FUNC 2
#define STT_OBJECT 1

using ByteVec = std::vector<u8>;

auto Code::create_dummy_code() -> Code {
	Code c;
	c.text = {0x48, 0xc7, 0xc0, 0x8b, 0x01, 0x00, 0x00, 0xc3,
		      0x48, 0xc7, 0xc0, 0x8d, 0x01, 0x00, 0x00, 0xc3};
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
	sec_tab.body(SectionType::SymTab) = (Serializer() << elf_syms).out;

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
	ser << elf_header;

	for (u16 sec_idx = 0; sec_idx < SectionTable::num_sections(); ++sec_idx) {
		SectionType sec_ty = SectionTable::sec_ty_from_idx(sec_idx);

		sec_tab.header(sec_ty).sh_offset = ser.out.size();
		ser << sec_tab.body(sec_ty);
	}

	for (u16 sec_idx = 0; sec_idx < SectionTable::num_sections(); ++sec_idx) {
		SectionType sec_ty = SectionTable::sec_ty_from_idx(sec_idx);
		ser << sec_tab.header(sec_ty);
	}

	File::write(ser.out.data(), ser.out.size(), fs::path("./elf_file_v2"));
}
