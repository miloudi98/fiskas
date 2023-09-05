#include <gtest/gtest.h>
#include <vector>

#include "asm_cc/lexer.hh"
#include "base.hh"
#include "fmt/format.h"

using CharVec = std::vector<char>;

#define COMPARE_TOKEN_KINDS(program, ...)                                               \
	do {																				\
		Lexer lexer(CharVec(program.begin(), program.end()));                           \
		std::vector<TokenKind> expected_tokens = { __VA_ARGS__ };                       \
		for (u32 i = 0; i < expected_tokens.size(); ++i) {								\
			EXPECT_EQ(lexer.next_token().kind, expected_tokens[i]);						\
		}																				\
	}																					\
	while(0)																			\


TEST(SmokeTest, SmokeTestNumber1) {
    std::string program = R"(
		// This is a comment and should be ignored.
		// This should not change anything.
		fn main() {
			mov(RAX, 12569);
		}
	)";

	COMPARE_TOKEN_KINDS(program,
			TokenKind::Fn,
			TokenKind::Identifier,
			TokenKind::LeftParen,
			TokenKind::RightParen,
			TokenKind::LeftBrace,
			TokenKind::Mnemonic,
			TokenKind::LeftParen,
			TokenKind::Identifier,
			TokenKind::Comma,
			TokenKind::Number,
			TokenKind::RightParen,
			TokenKind::SemiColon,
			TokenKind::RightBrace,
			TokenKind::Eof,
		);
}
