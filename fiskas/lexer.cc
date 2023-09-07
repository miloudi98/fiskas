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

auto Lexer::next_token() -> Token {
	eat_while(is_whitespace);

	if (eof()) return Token::gen(TokenKind::Eof, 0, 0);

	usz start_offset = pos();

	char c = next_char_pnc();
	TokenKind tok_kind = [&] {
		switch (c) {
			case '(': return TokenKind::LeftParen;
			case ')': return TokenKind::RightParen;
			case '{': return TokenKind::LeftBrace;
			case '}': return TokenKind::RightBrace;
			case ',': return TokenKind::Comma;
			case ';': return TokenKind::SemiColon;
			default: return multi_char_token_kind(); 
		}
		fiska_unreachable();
	}();

	if (tok_kind == TokenKind::Identifier) {
		std::string tok_literal = source_substring(start_offset, pos());
		tok_kind = keywords.contains(tok_literal) 
			? keywords.at(tok_literal) 
			: TokenKind::Identifier;
	}

	return Token::gen(tok_kind, start_offset, pos() - start_offset);
}

} // namespace lexer
} // namespace fiskas 
