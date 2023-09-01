#ifndef __FISKA_ASSEMBLER_ELF_SERIALIZER_HH__
#define __FISKA_ASSEMBLER_ELF_SERIALIZER_HH__

#include <vector>

#include "base.hh"

struct Serializer {
	std::vector<u8> out;

    template <typename T>
        requires ::detail::UnsignedInt<T>
    auto operator<<(T value) -> Serializer &;

    template <typename T>
    auto operator<<(const std::vector<T> &data) -> Serializer & {
		for (const auto &elem : data) {
			*this << elem;
		}
		return *this;
	}

    auto operator<<(std::span<const u8> data) -> Serializer &;

	auto operator<<(const ElfHeader &elf_header) -> Serializer &;

	auto operator<<(const Symbol &sym) -> Serializer &;

	auto operator<<(const SectionHeader &header) -> Serializer &;

};

#endif // __FISKA_ASSEMBLER_ELF_SERIALIZER_HH__
