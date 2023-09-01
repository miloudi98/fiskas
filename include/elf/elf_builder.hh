#ifndef __FISKA_ASSEMBLER_ELF_ELF_BUILDER_HH__
#define __FISKA_ASSEMBLER_ELF_ELF_BUILDER_HH__

#include <vector>

#include "base.hh"
#include "elf/elf_types.hh"

struct Code {
	std::vector<u8> text;
	std::vector<u8> data;

	struct Symbol {
		u64 offset{};
		SectionType code_section;
		std::string name{};
		u64 value{};
	};
	std::vector<Symbol> symbols;

public:
	static auto create_dummy_code() -> Code;
};

auto build_elf_file(const Code &code) -> void;

#endif  // __FISKA_ASSEMBLER_ELF_ELF_BUILDER_HH__
