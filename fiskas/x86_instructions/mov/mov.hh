#ifndef __FISKA_ASSEMBLER_FISKAS_X86_INSTRUCTIONS_MOV_PARSER_HH__
#define __FISKA_ASSEMBLER_FISKAS_X86_INSTRUCTIONS_MOV_PARSER_HH__

#include <vector>

#include "parser.hh"
#include "base.hh"

namespace fiskas {
namespace x86_instruction {

enum struct MovInstructionKind {
	// Mov reg reg
	RegToReg,
	// Mov mem reg
	RegToMem,
	// Mov reg mem
	MemToReg,
	// Mov reg imm
	ImmToReg,
	// Mov reg moffs
	MoffsToReg,
	// Mov moffs reg
	RegToMoffs
};

struct MovInstruction : parser::Instruction {
	MovInstruction(MovInstructionKind kind_) 
		: parser::Instruction(common::X86Mnemonic::Mov), kind(kind_) {}

	MovInstructionKind kind;
	auto encode() -> std::vector<u8>; 
};

struct MovInstructionParser {
	static auto parse(parser::Parser *parser) -> MovInstruction *;
	static auto next_register(parser::Parser *parser) -> common::Reg;
};

}
}

#endif // __FISKA_ASSEMBLER_FISKAS_X86_INSTRUCTIONS_MOV_PARSER_HH__
