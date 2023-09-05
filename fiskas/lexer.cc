#include "lexer.hh"

namespace fiskas {
namespace lexer {

const StringMap<TokenKind> Lexer::keywords = {
	{"fn", TokenKind::Fn}
};

auto str_of_token_kind(TokenKind kind) -> std::string {
	using enum TokenKind;
	switch (kind) {
		case Invalid: return "<Invalid>";
		case Eof: return "<Eof>";
		case LeftParen: return "<LeftParen>";
		case RightParen: return "<RightParen>";
		case LeftBrace: return "<LeftBrace>";
		case RightBrace: return "<RightBrace>";
		case Comma: return "<Comma>";
		case SemiColon: return "<SemiColon>";
		case Fn: return "<Fn>";
		case Number: return "<Number>";
		case Identifier: return "<Identifier>";
	}
	fiska_unreachable();
}

}
}
