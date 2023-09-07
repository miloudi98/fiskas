#ifndef __FISKA_ASSEMBLER_FISKAS_X86_COMMON_HH__
#define __FISKA_ASSEMBLER_FISKAS_X86_COMMON_HH__

#include <string>
#include <string_view>
#include <optional>

#include "base.hh"

namespace fiskas {
namespace common {

struct ModRm {
	constexpr static u8 register_addressing = 0b11;

public:
	u8 byte = 0;

	auto mod(u8 value) -> ModRm & {
		fiska_assert(value <= 3, "Mod value '{}' in ModRM byte can't be bigger than 0b11", value);
		byte |= (value << 6);
		return *this;
	}

	auto reg(u8 value) -> ModRm & {
		fiska_assert(value <= 7, "Reg value '{}' in ModRM byte can't be bigger than 0b111", value);
		byte |= (value << 3);
		return *this;
	}

	auto rm(u8 value) -> ModRm & {
		fiska_assert(value <= 7, "r/m value '{}' in ModRM byte can't be bigger than 0b111", value);
		byte |= value;
		return *this;
	}

	auto value() -> u8 { return byte; }
};

struct Rex {
	constexpr static u8 fixed_field = 0b0100 << 4;
	constexpr static u8 w_bit = 1 << 3;
	constexpr static u8 r_bit = 1 << 2;
	constexpr static u8 x_bit = 1 << 1;
	constexpr static u8 b_bit = 1 << 0;

public:
	u8 byte = 0;

	auto w(bool need_w_prefix) -> Rex & {
		byte |= need_w_prefix * w_bit;
		return *this;
	}

	auto r(bool need_r_prefix) -> Rex & {
		byte |= need_r_prefix * r_bit;
		return *this;
	}

	auto x(bool need_x_prefix) -> Rex & {
		byte |= need_x_prefix;
		return *this;
	}

	auto b(bool need_b_prefix) -> Rex & {
		byte |= need_b_prefix;
		return *this;
	}

	auto value() -> u8 {
		// We don't need the rex prefix in this case.
		if (byte == 0) return 0;
		
		return fixed_field | byte;
	}
};

enum struct X86Mnemonic {
    Mov,
    Ret
};
auto x86_mnemonic_of_str(std::string_view mnemonic) -> std::optional<X86Mnemonic>;
auto x86_mnemonic_of_str_pnc(std::string_view mnemonic) -> X86Mnemonic;
auto str_of_x86_mnemonic(X86Mnemonic mnemonic) -> std::string;

enum struct BitWidth : u16 {
    b8 = 8,
    b16 = 16,
    b32 = 32,
    b64 = 64
};
auto str_of_bit_width(BitWidth width) -> std::string;

enum struct RegName {
	// 64 bit GPRs
    Rax, Rbx, Rcx, Rdx, Rbp, Rsi,
    Rdi, Rsp, R8, R9, R10, R11,
    R12, R13, R14, R15,

	// 32 bit GPRs
	Eax, Ebx, Ecx, Edx, Ebp, Esi,
	Edi, Esp, R8d, R9d, R10d, R11d,
	R12d, R13d, R14d, R15d,

	// 8-bit GPRs
	Al, Cl, Dl, Bl, Ah, Ch, Dh,
	Bh, Spl, Bpl, Sil, Dil, R8b,
	R9b, R10b, R11b, R12b, R13b, R14b,
	R15b,

	// Segment registers
	Cs, Ds, Ss, Es, Fs, Gs,
};
auto str_of_reg_name(RegName reg_name) -> std::string;
auto reg_name_of_str_pnc(std::string_view reg_name) -> RegName; 
auto reg_name_of_str(std::string_view reg_name) -> std::optional<RegName>;
auto bit_width_of_reg_name(RegName reg_name) -> BitWidth;
auto index_of_reg_name(RegName reg_name) -> u8;
auto requires_rex_extension(RegName reg_name) -> bool;
auto is_segment_register(RegName reg_name) -> bool;


struct Reg {
    RegName name;
	BitWidth width;
};

} // namespace common
} // namespace fiskas

#endif // __FISKA_ASSEMBLER_FISKAS_X86_COMMON_HH__
