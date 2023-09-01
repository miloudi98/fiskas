#include <span>
#include <type_traits>
#include <vector>

#include "base.hh"
#include "elf/elf_types.hh"
#include "elf/serializer.hh"
#include "fmt/format.h"

using ByteVec = std::vector<u8>;

template <typename T>
	requires ::detail::UnsignedInt<T>
auto Serializer::operator<<(T value) -> Serializer & {
	for (u8 idx = 0; idx < sizeof(T); ++idx) {
		out.push_back(value & 0xffu);
		value >>= 8;
	}
	return *this;
}

auto Serializer::operator<<(std::span<const u8> data) -> Serializer & {
	::detail::extend(out, ByteVec(data.begin(), data.end()));
	return *this;
}

// clang-format off
auto Serializer::operator<<(const ElfHeader &elf_header) -> Serializer & {
	return *this << elf_header.e_ident
				 << elf_header.e_type
				 << elf_header.e_machine
				 << elf_header.e_version
				 << elf_header.e_entry
				 << elf_header.e_phoff
				 << elf_header.e_shoff
				 << elf_header.e_flags
				 << elf_header.e_ehsize
				 << elf_header.e_phentsize
				 << elf_header.e_phnum
				 << elf_header.e_shentsize
				 << elf_header.e_shnum
				 << elf_header.e_shstrndx;
}

auto Serializer::operator<<(const Symbol &sym) -> Serializer & {
	return *this << sym.st_name
				 << sym.st_info
				 << sym.st_other
				 << sym.st_shndx
				 << sym.st_value
				 << sym.st_size;
}

auto Serializer::operator<<(const SectionHeader &header) -> Serializer & {
	return *this << header.sh_name
				 << header.sh_type
				 << header.sh_flags
				 << header.sh_addr
				 << header.sh_offset
				 << header.sh_size
				 << header.sh_link
				 << header.sh_info
				 << header.sh_addralign
				 << header.sh_entsize;
}
// clang-format on
