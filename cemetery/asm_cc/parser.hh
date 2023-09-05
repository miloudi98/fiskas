#ifndef __FISKA_ASSEMBLER_ELF_PARSER_HH__
#define __FISKA_ASSEMBLER_ELF_PARSER_HH__

#include <vector>

#include "base.hh"
#include "asm_cc/lexer.hh"
#include "asm_cc/x86_common.hh"

struct Instruction {
	Mnemonic::Spelling mnemonic;
};

struct Mov_Instruction : Instruction {
	using Operand = std::variant<Reg, MemRef, Imm, Moffs>;

	Operand dst;
	Operand src;

public:
	Mov_Instruction() : Instruction(Mnemonic::Spelling::Mov) {}
};

struct Ret_Instruction : Instruction {

public:
	Ret_Instruction() : Instruction(Mnemonic::Spelling::Ret) {}
};

struct FuncDecl {
	std::string name{};
	std::vector<Instruction> body;
};

struct Parser : Lexer {
	explicit Parser(std::vector<char> content) : Lexer(content) {}

	auto parse_func_decl() -> FuncDecl;

	auto parse_instruction() -> Instruction;
};

#endif // __FISKA_ASSEMBLER_ELF_PARSER_HH__
