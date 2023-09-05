#include <string_view>

#include "base.hh"
#include "x86_common.hh"

namespace fiskas {
namespace common {

const StringMap<X86Mnemonic> mnemonics = {
	{"mov", X86Mnemonic::Mov},
	{"ret", X86Mnemonic::Ret}
};

auto x86_mnemonic_of_str(std::string_view mnemonic) -> X86Mnemonic {
	fiska_assert(mnemonics.contains(mnemonic), "Unrecognized mnemonic '{}'", mnemonic);
	return mnemonics.find(mnemonic)->second;
}

auto str_of_x86_mnemonic(X86Mnemonic mnemonic) -> std::string {
	using enum X86Mnemonic;
	switch (mnemonic) {
		case Mov: return "mov";
		case Ret: return "ret";
	}
	return "";
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

auto str_of_regname(RegName reg_name) -> std::string {
	using enum RegName;
	switch (reg_name) {
		case Rax: return "RAX";
		case Rbx: return "RBX";
		case Rcx: return "RCX";
		case Rdx: return "RDX";
		case Rbp: return "RBP";
		case Rsi: return "RSI";
		case Rdi: return "RDI";
		case Rsp: return "RSP";
		case Rip: return "RIP";
		case R8: return "R8";
		case R9: return "R9";
		case R10: return "R10";
		case R11: return "R11";
		case R12: return "R12";
		case R13: return "R13";
		case R14: return "R14";
		case R15: return "R15";
	}
	return "";
}

} // namespace common
} // namespace fiskas
