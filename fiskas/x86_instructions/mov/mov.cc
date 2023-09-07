#include <vector>

#include "base.hh"
#include "x86_instructions/mov/mov.hh"

namespace fiskas {
namespace x86_instruction {

struct MovRegToReg : MovInstruction {
	MovRegToReg(common::Reg dst_, common::Reg src_) 
		: MovInstruction(MovInstructionKind::RegToReg),
		  dst(dst_), src(src_) {}

	common::Reg dst;
	common::Reg src;

public:
	auto validate_semantics() -> void;
	auto encode() -> std::vector<u8>;
};

struct MovRegToMem : MovInstruction {
	MovRegToMem() : MovInstruction(MovInstructionKind::RegToMem) {}

	common::MemRef mem_ref;
	common::Reg reg;
}

auto MovRegToReg::validate_semantics() -> void {
	using ::detail::one_of;
	using common::is_segment_register;
	using enum common::BitWidth;
	using enum common::RegName;

	// if the widths are different, we must moving to/from a segment register. 
	// Otherwise this is an invalid mov instruction.
	if (dst.width != src.width) {
		fiska_assert(
				is_segment_register(src.name) or is_segment_register(dst.name),
				"Register size mismatch in mov instruction. src regsiter width = '{}' "
				"and dst register width = '{}' bits", +src.width, +dst.width);
		
		if (is_segment_register(src.name)) {
			fiska_assert(one_of(dst.width, b16, b32, b64),
					"Destination register must be either r16/32/64. Dst width = '{}' bits",
					+dst.width);

		} else {
			fiska_assert(one_of(src.width, b16, b64), 
					"Source register must be r16/64 when moving data to a segment register. "
					"Src width = '{}' bits", +src.width);
					
		}

	} else {
		// |src| and |dst| registers have the same width.

		u8 rex_prefix = common::Rex()
			.r(common::requires_rex_extension(src.name))
			.b(common::requires_rex_extension(dst.name))
			.value();

		// registers AH, BH, CH, DH can't be addressed when a REX prefix
		// is present.
		if (src.width == b8 and rex_prefix != 0) {
			fiska_assert((not one_of(src.name, Ah, Bh, Ch, Dh)) 
					and (not one_of(dst.name, Ah, Bh, Ch, Dh)),
					"Registers AH, BH, CH, DH can't be addressed when a REX prefix is "
					"present");
		}
	}
}

auto MovRegToReg::encode() -> std::vector<u8> {
	// Make sure we have a valid mov instruction.
	validate_semantics();
	// Op encoding.
	//
	// [MR] Operand1 (dst)    Operand2 (src)
	//       ModRm:r/m (w)     ModRm:reg (r)

	u8 modrm_byte = common::ModRm()
		.mod(common::ModRm::register_addressing)
		.rm(common::index_of_reg_name(dst.name))
		.reg(common::index_of_reg_name(src.name))
		.value();

	u8 rex_prefix = common::Rex()
		.w(std::max(+src.width, +dst.width) == 64)
		.r(common::requires_rex_extension(src.name))
		.b(common::requires_rex_extension(dst.name))
		.value();

	u8 opcode = [&] {
		using enum common::BitWidth;
		using common::is_segment_register;

		if (is_segment_register(src.name) or is_segment_register(dst.name)) {
			return is_segment_register(src.name) ? u8(0x8c) : u8(0x8e);
		}

		switch (src.width) {
			case b8:
				return u8(0x88);
			case b16:
			case b32:
			case b64:
				return u8(0x89);
		}

		fiska_unreachable();
	}();

	return {rex_prefix, opcode, modrm_byte};
}

auto MovInstruction::encode() -> std::vector<u8> {
	using enum MovInstructionKind;

	switch (kind) {
		case RegToReg: 
			return static_cast<MovRegToReg *>(this)->encode();

		default:
			fiska_todo("Other MovInstruction kinds are not handled yet");
	}

	fiska_unreachable();
}

auto MovInstructionParser::next_register(parser::Parser *parser) -> common::Reg {
	auto reg_tok = parser->next_token();
	auto reg_literal = parser->source_substring(
			reg_tok.offset, reg_tok.offset + reg_tok.len);

	common::RegName reg_name = common::reg_name_of_str_pnc(reg_literal);
	return {.name = reg_name, .width = common::bit_width_of_reg_name(reg_name)};
}

auto MovInstructionParser::parse(parser::Parser *parser) -> MovInstruction * {
	using common::Reg;
	using enum lexer::TokenKind;

	parser->consume_pnc(LeftParen);

	Reg dst_reg = next_register(parser);
	parser->consume_pnc(Comma);
	Reg src_reg = next_register(parser);

	parser->consume_pnc(RightParen, SemiColon);


	return new MovRegToReg(dst_reg, src_reg);
}


}
}
