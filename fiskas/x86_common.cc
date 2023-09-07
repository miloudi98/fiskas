#include <string_view>
#include <optional>
#include <vector>

#include "base.hh"
#include "x86_common.hh"

namespace fiskas {
namespace common {

const StringMap<X86Mnemonic> mnemonics = {
	{"mov", X86Mnemonic::Mov},
	{"ret", X86Mnemonic::Ret}
};

const StringMap<RegName> regnames = {
	{"rax", RegName::Rax},
	{"rbx", RegName::Rbx},
	{"rcx", RegName::Rcx},
	{"rdx", RegName::Rdx},
	{"rbp", RegName::Rbp},
	{"rsi", RegName::Rsi},
	{"rdi", RegName::Rdi},
	{"rsp", RegName::Rsp},
	{"r8", RegName::R8},
	{"r9", RegName::R9},
	{"r10", RegName::R10},
	{"r11", RegName::R11},
	{"r12", RegName::R12},
	{"r13", RegName::R13},
	{"r14", RegName::R14},
	{"r15", RegName::R15},
};

auto x86_mnemonic_of_str(std::string_view mnemonic) -> std::optional<X86Mnemonic> {
	auto mnemonic_it = mnemonics.find(mnemonic);
	if (mnemonic_it == mnemonics.end()) return std::nullopt;
	return mnemonic_it->second;
}

auto x86_mnemonic_of_str_pnc(std::string_view mnemonic) -> X86Mnemonic {
	fiska_assert(mnemonics.contains(mnemonic), "Unrecognized mnemonic '{}'", mnemonic);
	return mnemonics.find(mnemonic)->second;
}

auto str_of_x86_mnemonic(X86Mnemonic mnemonic) -> std::string {
	using enum X86Mnemonic;
	switch (mnemonic) {
		case Mov: return "mov";
		case Ret: return "ret";
	}
	fiska_unreachable();
}

auto str_of_bit_width(BitWidth width) -> std::string {
	using enum BitWidth;
	switch (width) {
		case b8: return "8b";
		case b16: return "16b";
		case b32: return "32b";
		case b64: return "64b";
	}
	return "";
}

auto str_of_reg_name(RegName reg_name) -> std::string {
	using enum RegName;
	switch (reg_name) {
		// 64-bit
		case Rax: return "RAX";
		case Rbx: return "RBX";
		case Rcx: return "RCX";
		case Rdx: return "RDX";
		case Rbp: return "RBP";
		case Rsi: return "RSI";
		case Rdi: return "RDI";
		case Rsp: return "RSP";
		case R8: return "R8";
		case R9: return "R9";
		case R10: return "R10";
		case R11: return "R11";
		case R12: return "R12";
		case R13: return "R13";
		case R14: return "R14";
		case R15: return "R15";

		// 32-bit
		case Eax: return "EAX";
		case Ebx: return "EBX";
		case Ecx: return "ECX";
		case Edx: return "EDX";
		case Ebp: return "EBP";
		case Esi: return "ESI";
		case Edi: return "EDI";
		case Esp: return "ESP";
		case R8d: return "R8D";
		case R9d: return "R9D";
		case R10d: return "R10D";
		case R11d: return "R11D";
		case R12d: return "R12D";
		case R13d: return "R13D";
		case R14d: return "R14D";
		case R15d: return "R15D";

		// Segment registers
		case Cs: return "CS";
		case Ds: return "DS";
		case Ss: return "SS";
		case Es: return "ES";
		case Fs: return "FS";
		case Gs: return "GS";

		// 8-bit registers
		case Al: return "AL";
		case Cl: return "CL";
		case Dl: return "DL";
		case Bl: return "BL";
		case Ah: return "AH";
		case Ch: return "CH";
		case Dh: return "DH";
		case Bh: return "BH";
		case Spl: return "SPL";
		case Bpl: return "BPL";
		case Sil: return "SIL";
		case Dil: return "DIL";
		case R8b: return "R8B";
		case R9b: return "R9B";
		case R10b: return "R10B";
		case R11b: return "R11B";
		case R12b: return "R12B";
		case R13b: return "R13B";
		case R14b: return "R14B";
		case R15b: return "R15B";
	}
	fiska_unreachable();
}

auto reg_name_of_str_pnc(std::string_view reg_name) -> RegName {
	fiska_assert(regnames.contains(reg_name), "Unrecognized register name '{}'", reg_name);
	return regnames.find(reg_name)->second;
}

auto reg_name_of_str(std::string_view reg_name) -> std::optional<RegName> {
	if (not regnames.contains(reg_name)) return std::nullopt;
	return regnames.find(reg_name)->second;
}

auto bit_width_of_reg_name(RegName reg_name) -> BitWidth {
	using enum BitWidth;
	using enum RegName;

	switch (reg_name) {
		case Rax:
		case Rbx:
		case Rcx:
		case Rdx:
		case Rbp:
		case Rsi:
		case Rdi:
		case Rsp:
		case R8: 
		case R9: 
		case R10: 
		case R11: 
		case R12: 
		case R13: 
		case R14: 
		case R15: 
			return b64;

		case Eax:
		case Ebx:
		case Ecx:
		case Edx:
		case Ebp:
		case Esi:
		case Edi:
		case Esp:
		case R8d:
		case R9d:
		case R10d:
		case R11d:
		case R12d:
		case R13d:
		case R14d:
		case R15d:
			return b32;

		case Cs:
		case Ds:
		case Ss:
		case Es:
		case Fs:
		case Gs:
			return b16;

		case Al:
		case Cl:
		case Dl:
		case Bl:
		case Ah:
		case Ch:
		case Dh:
		case Bh:
		case Spl:
		case Bpl:
		case Sil:
		case Dil:
		case R8b:
		case R9b:
		case R10b:
		case R11b:
		case R12b:
		case R13b:
		case R14b:
		case R15b:
			return b8;
	}
	fiska_unreachable();
}

auto index_of_reg_name(RegName reg_name) -> u8 {
	using enum RegName;
	
	switch (reg_name) {
		case Rax: 
		case Eax:
		case Al:
		case R8:
		case R8d:
		case R8b:
		case Es:
			return 0;

		case Rcx:
		case Ecx:
		case Cl:
		case R9:
		case R9d:
		case R9b:
		case Cs:
			return 1;

		case Rdx:
		case Edx:
		case Dl:
		case R10:
		case R10d:
		case R10b:
		case Ss:
			return 2;

		case Rbx:
		case Ebx:
		case Bl:
		case R11:
		case R11d:
		case R11b:
		case Ds:
			return 3;

		case Rsp:
		case Esp:
		case Ah:
		case Spl:
		case R12:
		case R12d:
		case R12b:
		case Fs:
			return 4;

		case Rbp:
		case Ebp:
		case Ch:
		case Bpl:
		case R13:
		case R13d:
		case R13b:
		case Gs:
			return 5;

		case Rsi:
		case Esi:
		case Dh:
		case Sil:
		case R14:
		case R14d:
		case R14b:
			return 6;

		case Rdi:
		case Edi:
		case Dil:
		case Bh:
		case R15:
		case R15d:
		case R15b:
			return 7;
	}

	fiska_unreachable();
}

auto requires_rex_extension(RegName reg_name) -> bool {
	using enum RegName;

	switch (reg_name) {
		case Spl:
		case Bpl:
		case Sil:
		case Dil:
		case R8:
		case R9:
		case R10:
		case R11:
		case R12:
		case R13:
		case R14:
		case R15:
		case R8d:
		case R9d:
		case R10d:
		case R11d:
		case R12d:
		case R13d:
		case R14d:
		case R15d:
		case R8b:
		case R9b:
		case R10b:
		case R11b:
		case R12b:
		case R13b:
		case R14b:
		case R15b:
			return true;

		case Rax:
		case Eax:
		case Rcx:
		case Ecx:
		case Rdx:
		case Edx:
		case Rbx:
		case Ebx:
		case Rsp:
		case Esp:
		case Rbp:
		case Ebp:
		case Rsi:
		case Esi:
		case Rdi:
		case Edi:
		case Cs:
		case Ds:
		case Ss:
		case Es:
		case Fs:
		case Gs:
		case Al:
		case Cl:
		case Dl:
		case Bl:
		case Ah:
		case Ch:
		case Dh:
		case Bh:
			return false;
	}
	fiska_unreachable();
}

auto is_segment_register(RegName reg_name) -> bool {
	using enum RegName;

	// We're not doing a switch statement here because this 
	// function will not change if we add new registers in the future.
	//
	// An exhaustive switch statement allows the compiler to tell you what switch
	// statements you need to update once you add a new enum variant.
	return ::detail::one_of(reg_name, Cs, Ds, Ss, Es, Fs, Gs);
}

} // namespace common
} // namespace fiskas
