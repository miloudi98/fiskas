#ifndef __FISKA_ASSEMBLER_FISKAS_PARSER_HH__
#define __FISKA_ASSEMBLER_FISKAS_PARSER_HH__

#include "lexer.hh"
#include "base.hh"
#include "x86_common.hh"

namespace fiskas {
namespace parser {

struct Instruction {
	common::X86Mnemonic mnemonic;

	auto encode() -> std::vector<u8>;
};

struct FuncDecl {
	std::string name;
	std::vector<Instruction> body;
};

struct Parser : lexer::Lexer {
	Parser(std::string source) : Lexer(source) {}

	auto parse_func_decl() -> FuncDecl;


public:
	template <typename... TokenKinds>
	auto consume_pnc(const TokenKinds&... tok_kinds) -> void {
		auto check_if_tok_kind_matches = [this](const lexer::TokenKind &tok_kind) {
			fiska_assert(next_token().kind == tok_kind);
		};

		(check_if_tok_kind_matches(tok_kinds) , ...);
	}

	template <typename... TokenKinds>
	auto consume(const TokenKinds&... tok_kind) -> bool {
		return ((next_token().kind == tok_kind) and ...);
	}
};

} // namespace parser
} // namespace fiskas 

#endif // __FISKA_ASSEMBLER_FISKAS_PARSER_HH__
