#include "base.hh"
#include "elf/elf_builder.hh"
#include "fmt/format.h"
#include "fiskas/x86_common.hh"

auto main(i32 argc, char *argv[]) -> i32 {
	fiska_todo();
	build_elf_file(Code::create_dummy_code());
    fmt::print("Bismillah\n");

	fiskas::common::x86_mnemonic_of_str("this is not a mnemonic");
    return 0;
}
