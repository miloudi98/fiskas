#include "elf/elf_types.hh"

// clang-format off
const std::unordered_map<SectionType, const char *> SectionTable::section_names = {
    {SectionType::Null, ""},
    {SectionType::Text, ".text"},
    {SectionType::SectionHeaderStrTab, ".shstrtab"},
    {SectionType::SymTab, ".symtab"},
    {SectionType::Data, ".data"},
    {SectionType::SymTabStrTab, ".strtab"}
};
// clang-format on
