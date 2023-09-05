#ifndef __FISKA_ASSEMBLER_ASM_CC_LEXER_HH__
#define __FISKA_ASSEMBLER_ASM_CC_LEXER_HH__

#include <optional>
#include <vector>
#include <iostream>
#include <queue>

#include "asm_cc/lexer.hh"
#include "base.hh"

enum struct TokenKind {
	Invalid,
	Eof,

	LeftParen,
	RightParen,
	LeftBrace,
	RightBrace,
	SemiColon,
	Comma,

	Fn,
	Mnemonic,

	Number,
	Identifier,
};

struct Token {
	TokenKind kind = TokenKind::Invalid;
	u64 offset{};
	std::string literal{};
};

struct Lexer {
	using CharVec = std::vector<char>;

    CharVec file_content;
    const char *curr{};
    const char *end{};
	std::queue<Token> lookahead_toks{};

public:

	Lexer(CharVec content) : file_content(content),
				curr(file_content.data()), 
				end(file_content.data() + file_content.size())
	{}

	auto current_offset() -> u64 {
		return u64(curr - file_content.data());
	}

	auto next_char() -> std::optional<char> {
		if (reached_eof()) return std::nullopt;
		return *curr++;
	}

	auto next_char_pnc() -> char { return next_char().value(); }

	auto peek_char() -> std::optional<char> {
		if (reached_eof()) return std::nullopt;
		return *curr;
	}

	auto peek_char_pnc() -> char { return peek_char().value(); }

	auto reached_eof() -> bool { return curr >= end; }

	// Rename this to something clearer.
	auto expect_and_consume(auto... c) -> bool {
		auto check_char = [this](char c) {
			auto cc = next_char();
			// AHYA(miloudi): replace this error-reporting logic with a custom assert that shows a good error message
			// plus a stack trace if possible.
			if ((not cc.has_value()) or cc.value() != c) {
				std::string curr_char = cc.has_value() ? ""s + *cc : "None";
				fmt::print("Fatal error: ");
				fmt::print("Expected char '{}' but got '{}'.\n", c, curr_char); 
				std::exit(1);
			}
			return true;
		};
		return (check_char(c) and ...);
	}

	auto substring(u64 beg, u64 end) -> std::string {
		u64 count = end - beg;
		return std::string(file_content.data() + beg, count);
	}

	template <typename UnaryPredicate>
	auto consume_while(UnaryPredicate p) {
		while (peek_char().has_value() and p(peek_char_pnc())) {
			next_char();
		}
	}

	auto fetch_token() -> Token;

	auto next_token() -> Token;

	auto peek_token() -> Token;

	auto consume_tok_pnc(TokenKind kind) -> Token;

	static auto is_whitespace(char c) -> bool { return (c == ' ') or (c == '\r') or (c == '\t') or (c == '\n'); }

	static auto is_decimal_digit(char c) -> bool { return (c >= '0' and c <= '9'); }

	static auto is_hex_digit(char c) -> bool { return is_decimal_digit(c) or (c >= 'a' and c <= 'f') or (c == '_'); }

	static auto is_alpha(char c) -> bool { return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z'); }

	static auto is_identifier_start(char c) -> bool { return is_alpha(c); }
	
	static auto can_continue_identifier(char c) -> bool { return is_alpha(c) or is_decimal_digit(c) or c == '_'; }

	static auto is_decimal_digit_start(char c) -> bool { return is_decimal_digit(c) or c == '-'; }

	static auto is_comment_start(char c) -> bool { return c == '/'; }

};

#endif  // __FISKA_ASSEMBLER_ASM_CC_LEXER_HH__
