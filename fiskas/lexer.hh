#ifndef __FISKA_ASSEMBLER_FISKAS_LEXER_HH__
#define __FISKA_ASSEMBLER_FISKAS_LEXER_HH__

#include <optional>
#include <string>
#include <functional>

#include "base.hh"

namespace fiskas {
namespace lexer {

struct Cursor {
    const std::string source{};
    const char *curr{};
    char prev = ' ';

public:
	static auto is_number(char c) -> bool { return (c >= '0' and c <= '9'); }
	static auto is_ident_start(char c) -> bool { return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z'); }
	static auto is_ident_continue(char c) -> bool { return is_ident_start(c) or (c == '_') or is_number(c); }
	static auto is_whitespace(char c) -> bool { return (c == '\n') or (c == ' ') or (c == '\t') or (c == '\r'); }

public:
    Cursor(std::string source_) : source(source_), curr(source.data()) {}

    // Current offset into the program source code string.
    auto pos() -> usz { return usz(curr - source.data()); }

    auto eof() -> bool { return pos() >= source.size(); }

    auto next_char() -> std::optional<char> {
        if (eof()) return std::nullopt;

        prev = *curr++;
        return prev;
    }
    auto next_char_pnc() -> char {
        fiska_assert(!eof(), "Requesting the next char after reaching eof");
        return next_char().value();
    }

    auto peek_char() -> std::optional<char> {
        if (eof()) return std::nullopt;
        return *curr;
    }
    auto peek_char_pnc() -> char {
        fiska_assert(!eof(), "Peeking the next char after reaching eof");
        return peek_char().value();
    }

	auto eat_while(std::function<bool(char)> p) -> void {
		while (peek_char().has_value() and p(peek_char_pnc())) {
			next_char_pnc();
		}
	}

	auto source_substring(usz start, usz end) -> std::string {
		usz len = end - start;
		return source.substr(start, len);
	}
};

enum struct TokenKind {
	Invalid,
	Eof,

	// One char tokens.
	LeftParen,
	RightParen,
	LeftBrace,
	RightBrace,
	SemiColon,
	Comma,

	// Keywords.
	Fn,

	Number,
	Identifier,
};
auto str_of_token_kind(TokenKind kind) -> std::string;

struct Token {
	TokenKind kind = TokenKind::Invalid;
	usz offset{};
	usz len{};

	static auto gen(TokenKind kind, usz offset, usz len) -> Token {
		return {.kind = kind, .offset = offset, .len = len};
	}
	static auto gen(TokenKind kind, usz len) -> Token {
		return {.kind = kind, .len = len};
	}
};

struct Lexer : Cursor {
public:
	static const StringMap<TokenKind> keywords;

public:
	Lexer(std::string source) : Cursor(source) {}

	auto next_token() -> Token {
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

	auto multi_char_token_kind() -> TokenKind {
		if (is_number(prev)) {
			eat_while(is_number);
			return TokenKind::Number;
		}

		if (is_ident_start(prev)) {
			eat_while(is_ident_continue);
			return TokenKind::Identifier;
		}

		fiska_unreachable("Char '{}' does not start a number nor an identifier", prev);
	}
};

} // namespace lexer
} // namespace fiskas

#endif // __FISKA_ASSEMBLER_FISKAS_LEXER_HH__
