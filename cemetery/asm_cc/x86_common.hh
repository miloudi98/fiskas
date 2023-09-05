#ifndef __FISKA_ASSEMBLER_ASM_CC_X86_COMMON_HH__
#define __FISKA_ASSEMBLER_ASM_CC_X86_COMMON_HH__

#include <unordered_map>

#include "base.hh"

struct Mnemonic {
	enum struct Spelling {
		Mov,
		Ret,
	};
	Spelling spelling;

public:
	static auto spelling_of_str(std::string_view mnemonic_name) -> Mnemonic::Spelling;
	static auto str_of_spelling(Mnemonic::Spelling spelling) -> std::string;

public:
	const static StringMap<Mnemonic::Spelling> str_mnemonic_spelling_mapping;
};

enum struct RegLabel {
	// 64 Bits.
	Rax, Rbx, Rcx, Rdx, Rbp, Rsi, Rdi,
	Rsp, Rip, R8, R9, R10, R11, R12,
	R13, R14, R15,

	// 32 bits.
	//Eax, Ebx, Ecx, Edx, Ebp, Esi, Edi, 
	//Esp, Eip, R8d, R9d, R10d, R11d, R12d,
	//R13d, R14d, R15d,

	//// 16 bits.
	//Ax, Bx, Cx, Dx, Bp, Si, Di, Sp,
	//R8w, R9w, R10w, R11w, R12w, R13w,
	//R14w, R15w,

	//// 8 bits.
	//Al, Ah, Bl, Bh, Cl, Ch, Dl, Dh,
	//Bpl, Sil, Dil, Spl, R8b, R9b,
	//R10b, R11b, R12b, R13b, R14b,
	//R15b,

	//// Segment registers
	//Cs, Ds, Ss, Es, Fs, Gs,

	//// EFlags register
	//Eflags,
};

struct Reg {
	RegLabel label;

public:
	Reg(RegLabel label_) : label(label_) {}
	Reg() {}

	// AHYA(miloudi): fix this.
	auto size() -> u16 {
		return 64;
	};

	auto is_seg_reg() -> bool {
		return false;
	}

	auto requires_rex() -> bool {
		return label >= RegLabel::R8 and label <= RegLabel::R15;
	}

//public:
//
//	static auto str_of_label(RegLabel label) -> std::string;
//	static auto label_of_str(std::string_view reg_name) -> Reg::Label;
//
//public:
//	const static std::unordered_map<RegLabel, u32> reg_label_width_mapping;
//	const static StringMap<RegLabel> str_reg_label_mapping;
};

struct MemRef {
	enum struct Scale {
		One = 1,
		Two = 2,
		Four = 4,
		Eight = 8,
	};

	Reg base;
	Reg index;
	Scale scale = Scale::One;

public:
	constexpr static u8 type_id = 1;
};

struct Imm {
	u64 value{};

public:
	constexpr static u8 type_id = 2;
};

struct Moffs {
	u64 value{};

public:
	constexpr static u8 type_id = 3;
};

#endif // __FISKA_ASSEMBLER_ASM_CC_X86_COMMON_HH__
