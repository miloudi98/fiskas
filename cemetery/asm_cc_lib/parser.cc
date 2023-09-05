#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <string>

#include "asm_cc/parser.hh"
#include "base.hh"

auto Parser::parse_func_decl() -> FuncDecl {
	consume_tok_pnc(TokenKind::Fn);

	Token func_name = consume_tok_pnc(TokenKind::Identifier);

	consume_tok_pnc(TokenKind::LeftParen);
	consume_tok_pnc(TokenKind::RightParen);
	consume_tok_pnc(TokenKind::LeftBrace);

	std::vector<Instruction> instrs;
	while (peek_token().kind != TokenKind::RightBrace) {
		instrs.push_back(parse_instruction());
	}

	consume_tok_pnc(TokenKind::RightBrace);

	return (FuncDecl) {
		.name = func_name.literal,
		.body = instrs
	};
}

auto Parser::parse_instruction() -> Instruction {
	//auto mnemonic = consume_tok_pnc(TokenKind::Mnemonic);
	//auto spelling = Mnemonic::spelling_of_str(mnemonic.literal);
	//consume_tok_pnc(TokenKind::LeftParen);

	//if (spelling == Mnemonic::Spelling::Mov) {
	//	Mov_Instruction mov_instr{};

	//	while (peek_token().kind != TokenKind::RightParen) {
	//		auto op = next_token();

	//		if (op.kind == TokenKind::Number) {
	//			mov_instr.operands.push_back(std::stoll(op.literal));

	//		} else if (op.kind == TokenKind::Identifier) {
	//			auto reg_label = Reg::label_of_str(op.literal);
	//			mov_instr.operands.push_back(Reg(reg_label));

	//		} else {
	//			fmt::print("Unknown token received when trying to parse Mov instruction operand\n");
	//			std::exit(1);
	//		}

	//		if (peek_token().kind == TokenKind::Comma) {
	//			next_token();
	//		}
	//	}

	//	consume_tok_pnc(TokenKind::RightParen);
	//	consume_tok_pnc(TokenKind::SemiColon);
	//	return mov_instr;
	//}

	//if (spelling == Mnemonic::Spelling::Ret) {
	//	consume_tok_pnc(TokenKind::SemiColon);
	//	return Ret_Instruction{};
	//}

	//fmt::print("Unknown instruction\n");
	//std::exit(1);
	return Ret_Instruction{};
}
