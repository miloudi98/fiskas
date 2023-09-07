#include <gtest/gtest.h>

#include "lexer.hh"

namespace fiskas {
namespace lexer {
namespace test {

auto check_toks_eq(std::string_view program, std::vector<Token> expected_toks) -> void {
	Lexer lexer{std::string(program)};
	for (usz i = 0; i < expected_toks.size(); ++i) {
		auto tok = lexer.next_token();
		EXPECT_EQ(expected_toks[i].kind, tok.kind);
		EXPECT_EQ(expected_toks[i].len, tok.len);
	}
}

TEST(SmokeTest, SmokeTest_1) {
	using enum TokenKind;

    std::string program = R"(
	fn main() {
		mov(RAX, RBX);
	}
	)";

	check_toks_eq(program, { 
		Token::gen(Fn, 2),
		Token::gen(Identifier, 4),
		Token::gen(LeftParen, 1),
		Token::gen(RightParen, 1),
		Token::gen(LeftBrace, 1),
		Token::gen(Identifier, 3),
		Token::gen(LeftParen, 1),
		Token::gen(Identifier, 3),
		Token::gen(Comma, 1),
		Token::gen(Identifier, 3),
		Token::gen(RightParen, 1),
		Token::gen(SemiColon, 1),
		Token::gen(RightBrace, 1),
		Token::gen(Eof, 0, 0)
	});

}

TEST(SmokeTest, SmokeTest_2) {
	using enum TokenKind;

	std::string program = R"(
	fn start() {
		mov(RAX, RBX);
		mov(R8, R9);
		mov(R10, R12);
	}
	)";

	check_toks_eq(program, {
		Token::gen(Fn, 2),
		Token::gen(Identifier, 5),
		Token::gen(LeftParen, 1),
		Token::gen(RightParen, 1),
		Token::gen(LeftBrace, 1),

		Token::gen(Identifier, 3),
		Token::gen(LeftParen, 1),
		Token::gen(Identifier, 3),
		Token::gen(Comma, 1),
		Token::gen(Identifier, 3),
		Token::gen(RightParen, 1),
		Token::gen(SemiColon, 1),

		Token::gen(Identifier, 3),
		Token::gen(LeftParen, 1),
		Token::gen(Identifier, 2),
		Token::gen(Comma, 1),
		Token::gen(Identifier, 2),
		Token::gen(RightParen, 1),
		Token::gen(SemiColon, 1),

		Token::gen(Identifier, 3),
		Token::gen(LeftParen, 1),
		Token::gen(Identifier, 3),
		Token::gen(Comma, 1),
		Token::gen(Identifier, 3),
		Token::gen(RightParen, 1),
		Token::gen(SemiColon, 1),

		Token::gen(RightBrace, 1),
		Token::gen(Eof, 0, 0)
	});
}

TEST(SmokeTest, SmokeTest_3) {
	using enum TokenKind;

	std::string program = R"(
	fn start() {
		mov(RAX, 10123123);
	}
	)";

	check_toks_eq(program, {
		Token::gen(Fn, 2),
		Token::gen(Identifier, 5),
		Token::gen(LeftParen, 1),
		Token::gen(RightParen, 1),
		Token::gen(LeftBrace, 1),

		Token::gen(Identifier, 3),
		Token::gen(LeftParen, 1),
		Token::gen(Identifier, 3),
		Token::gen(Comma, 1),
		Token::gen(Number, 8),
		Token::gen(RightParen, 1),
		Token::gen(SemiColon, 1),

		Token::gen(RightBrace, 1),
		Token::gen(Eof, 0, 0)
	});
}


}
} // namespace lexer
} // namespace fiskas
