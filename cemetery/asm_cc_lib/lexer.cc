#include <unordered_map>
#include <vector>
#include <string>

#include "asm_cc/lexer.hh"
#include "base.hh"

// clang-format off
const std::unordered_map<std::string, TokenKind> keywords = {
	{"fn"s, TokenKind::Fn},
	{"mov"s, TokenKind::Mnemonic},
	{"return"s, TokenKind::Mnemonic},
};
// clang-format on

auto tok_kind_to_string(TokenKind kind) -> std::string {
	using enum TokenKind;

	switch(kind) {
		case Invalid:
			return "<Invalid>";
		case Eof:
			return "<Eof>";
		case LeftParen:
			return "LeftParen";
		case RightParen:
			return "RightParen";
		case LeftBrace:
			return "LeftBrace";
		case RightBrace:
			return "RightBrace";
		case SemiColon:
			return "SemiColon";
		case Comma:
			return "Comma";
		case Fn:
			return "Fn";
		case Mnemonic:
			return "Mnemonic";
		case Number:
			return "Number";
		case Identifier:
			return "Identifier";
		default:
			return "<UNKOWN>";
	}
}

auto Lexer::consume_tok_pnc(TokenKind kind) -> Token {
	auto tok = next_token();
	if (tok.kind != kind) {
		fmt::print("Fatal error: expected token of kind '{}' but found one of kind '{}'.\n",
				tok_kind_to_string(kind), tok_kind_to_string(tok.kind));
		std::exit(1);
	}
	return tok;
}

auto char_to_str(char c) -> std::string {
	return fmt::format("{}", c);
}

auto Lexer::next_token() -> Token {
	if (lookahead_toks.empty()) {
		return fetch_token();
	}

	Token ret = lookahead_toks.front();
	lookahead_toks.pop();
	return ret;
}

auto Lexer::peek_token() -> Token {
	if (lookahead_toks.empty()) {
		lookahead_toks.push(fetch_token());
	}

	return lookahead_toks.front();
}

auto Lexer::fetch_token() -> Token {
	consume_while(is_whitespace);

	if (reached_eof()) {
		return { .kind = TokenKind::Eof, .offset = current_offset() };
	}

	if (is_comment_start(peek_char_pnc())) {
		// consume entire line.
		consume_while([](char c) { return c != '\n'; });
		return fetch_token();
	}

	auto c = next_char_pnc();

	switch(c) {
		case '(':
			return (Token) {
				.kind = TokenKind::LeftParen,
				.offset = current_offset() - 1,
				.literal = char_to_str(c)
			};
		case ')':
			return (Token) {
				.kind = TokenKind::RightParen,
				.offset = current_offset() - 1,
				.literal = char_to_str(c)
			};
		case '{':
			return (Token) {
				.kind = TokenKind::LeftBrace,
				.offset = current_offset() - 1,
				.literal = char_to_str(c)
			};
		case '}':
			return (Token) {
				.kind = TokenKind::RightBrace,
				.offset = current_offset() - 1,
				.literal = char_to_str(c)
			};
		case ';':
			return (Token) {
				.kind = TokenKind::SemiColon,
				.offset = current_offset() - 1,
				.literal = char_to_str(c)
			};
		case ',':
			return (Token) {
				.kind = TokenKind::Comma,
				.offset = current_offset() - 1,
				.literal = char_to_str(c)
			};
		default:
			if (is_identifier_start(c)) {
				auto start = current_offset() - 1;
				consume_while(can_continue_identifier);
				auto end = current_offset();

				auto tok_literal = substring(start, end);

				auto tok_kind = keywords.contains(tok_literal)
					? keywords.at(tok_literal)
					: TokenKind::Identifier;

				return {
					.kind = tok_kind,
					.offset = start,
					.literal = tok_literal
				};
			}

			if (is_decimal_digit_start(c)) {
				auto start = current_offset() - 1;
				consume_while(is_decimal_digit);
				auto end = current_offset();

				return {
					.kind = TokenKind::Number,
					.offset = start,
					.literal = substring(start, end)
				};
			}

			fmt::print("Lexer encountered unknown character '{}'\n", c);
			std::exit(1);
	}
}
