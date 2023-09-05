#include <gtest/gtest.h>

#include "asm_cc/parser.hh"

TEST(ParserSmokeTest, SmokeTestNumber1) {
    std::string program = R"(
		// This is a comment and should be ignored.
		// This should not change anything.
		fn main() {
			mov(RAX, 12569);
		}
	)";

	//std::vector<char> content(program.begin(), program.end());
	//FuncDecl func = Parser(content).parse_func_decl();

	//EXPECT_EQ(func.name, "main");

	//std::vector<Operand::Kind> expected_operand_kind = {
	//	Operand::Kind::Reg,
	//	Operand::Kind::Imm,
	//};

	//EXPECT_EQ(expected_operand_kind.size(), func.body[0].operands.size());

	//for (u32 idx = 0; idx < func.body[0].operands.size(); ++idx) {
	//	EXPECT_EQ(expected_operand_kind[idx], func.body[0].operands[idx].kind);
	//}
}
