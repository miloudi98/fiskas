#include "base.hh"
#include <concepts>
#include <iterator>
#include <type_traits>
#include <vector>

// (!) All this stuff can be generated at compile time. Copy the logic from TargetMachine in
// LensorCompiler.

// I don't really need a program header because in my case I am only going to output a relocatable
// object file that will then be linked together with my cpp file in order to validate the assembly,
// nothing more nothing less.

// This is extremely horrible mate.
// Macros taken from the Linux source code.
#define EI_NIDENT 16

#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define EI_CLASS 4
#define EI_DATA 5
#define EI_VERSION 6
#define EI_OSABI 7
#define EI_PAD 8

#define ELFMAG0 0x7f
#define ELFMAG1 0x45
#define ELFMAG2 0x4c
#define ELFMAG3 0x46
#define ELFCLASS64 2
#define ELFDATA2LSB 1
#define EV_CURRENT 1
#define ELFOSABI_LINUX 3

// Elf file types.
#define ET_NONE 0
#define ET_REL 1
#define ET_EXEC 2
#define ET_DYN 3
#define ET_CORE 4

// Elf machine type. We only care about AMD x64 at the moment.
// AMD x86-64
#define EM_X86_64 62

// Special index to specify that this ELF file does not have a section name string table.
#define SHN_UNDEF 0

// clean this mess up please.
template <typename T, typename... U>
concept is_any_of = (... or std::same_as<T, U>);

template <typename num>
    requires is_any_of<num, u8, u16, u32, u64>
std::vector<u8> serialize(num x) {
    std::vector<u8> ret;
    for (u32 i = 0; i < sizeof x; ++i) {
        ret.push_back(x & 0xffu);
        x >>= 8;
    }
    return ret;
}

template <u32 n>
auto serialize(const u8 (&arr)[n]) -> std::vector<u8> { return std::vector<u8>(arr, arr + n); }

template <typename T>
    requires std::same_as<T, u8>
void extend(std::vector<T> &dst, const std::vector<T> &src) {
    std::copy(src.begin(), src.end(), std::back_inserter(dst));
}

namespace fiska {
namespace elf {

// ElfHeader.
struct ElfHeader {
    u8 e_ident[EI_NIDENT]{};
    u16 e_type{};
    u16 e_machine{};
    u32 e_version{};
    u64 e_entry{};
    u64 e_phoff{};
    u64 e_shoff{}; // Section header table offset.
    u32 e_flags{};
    u16 e_ehsize{};
    u16 e_phentsize{};
    u16 e_phnum{};
    u16 e_shentsize{}; // Section header entry size.
    u16 e_shnum{};     // Number of sections headers.
    u16 e_shstrndx{};  // Index of the section header string table in the section header table.

public:
    ElfHeader() {
        e_ident[EI_MAG0] = ELFMAG0;
        e_ident[EI_MAG1] = ELFMAG1;
        e_ident[EI_MAG2] = ELFMAG2;
        e_ident[EI_MAG3] = ELFMAG3;
        e_ident[EI_CLASS] = ELFCLASS64;
        e_ident[EI_DATA] = ELFDATA2LSB;
        e_ident[EI_VERSION] = EV_CURRENT;
        e_ident[EI_OSABI] = ELFOSABI_LINUX;

        // Relocatable file.
        e_type = ET_REL;
        e_machine = EM_X86_64;
        e_ehsize = 64;
		e_shentsize = 64;
    }
};

// sh_type values.
#define SHT_NULL 0
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6

// sh_flags
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2

template <typename T>
struct SectionHeader {
	using section_type = T;

    u32 sh_name{}; // depends on the string table being written
    u32 sh_type{};
    u64 sh_flags{};
    u64 sh_addr{};
    u64 sh_offset{}; // offset into the file. Depends on the entire section being written.
    u64 sh_size{};
    u32 sh_link{};
    u32 sh_info{};
    u64 sh_addralign{};
    u64 sh_entsize{};
};

//struct TextSection {
//    std::vector<u8> bytes = {0x48, 0xc7, 0xc0, 0x7b, 0x00, 0x00, 0x00, 0xc3};
//};

} // namespace elf
} // namespace fiska
