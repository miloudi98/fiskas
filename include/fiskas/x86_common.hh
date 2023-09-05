#ifndef __FISKA_ASSEMBLER_FISKAS_X86_COMMON_HH__
#define __FISKA_ASSEMBLER_FISKAS_X86_COMMON_HH__

#include <string>
#include <string_view>

#include "base.hh"

namespace fiskas {
namespace common {

enum struct X86Mnemonic {
    Mov,
    Ret
};
auto x86_mnemonic_of_str(std::string_view mnemonic) -> X86Mnemonic;
auto str_of_x86_mnemonic(X86Mnemonic mnemonic) -> std::string;

enum struct BitWidth : u16 {
    b8 = 8,
    b16 = 16,
    b32 = 32,
    b64 = 64
};
auto str_of_bit_width(BitWidth width) -> std::string;

enum struct RegName {
    Rax, Rbx, Rcx, Rdx, Rbp, Rsi,
    Rdi, Rsp, Rip, R8, R9, R10,
    R11, R12, R13, R14, R15,
};
auto str_of_regname(RegName reg_name) -> std::string;

struct Reg {
    RegName name;
	BitWidth width;
};

} // namespace common
} // namespace fiskas

#endif // __FISKA_ASSEMBLER_FISKAS_X86_COMMON_HH__
