#include "base.hh"
#include "elf/elf_builder.hh"
#include "fmt/format.h"

auto main(i32 argc, char *argv[]) -> i32 {
	build_elf_file(Code::create_dummy_code());
    fmt::print("Bismillah\n");
    return 0;
}
