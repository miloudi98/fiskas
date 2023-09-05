#include <unordered_map>
#include <vector>

#include "asm_cc/x86_common.hh"

//using ConstRegRef = const Reg &;
//using ConstMemRef = const MemRef &;
//using ByteVec = std::vector<u8>;
//
//const std::unordered_map<Reg::Label, BitWidth> reg_label_width_mapping = {
//	{Reg::Label::Rax, BitWidth::w64},
//	{Reg::Label::Rbx, BitWidth::w64},
//	{Reg::Label::Rcx, BitWidth::w64},
//};
//
//const StringMap<Reg::Label> str_reg_label_mapping = {
//	{"rax", Reg::Label::Rax},
//	{"rbx", Reg::Label::Rbx},
//	{"rcx", Reg::Label::Rcx}
//};
//
//const StringMap<Mnemonic::Spelling> str_mnemonic_spelling_mapping = {
//	{"mov", Mnemonic::Spelling::Mov},
//	{"ret", Mnemonic::Spelling::Ret},
//};
//
//auto Reg::size() -> u32 {
//	// Debug
//	if (not reg_label_width_mapping.contains(label)) {
//		fmt::print("'reg_label_width_mapping' is out of sync with the reg labels\n");
//		std::exit(1);
//	}
//
//	return reg_label_width_mapping.at(label);
//}
//
//auto Reg::size(Reg::Label reg_label) -> u32 {
//	// Debug
//	if (not reg_label_width_mapping.contains(reg_label)) {
//		fmt::print("'reg_label_width_mapping' is out of sync with the reg labels\n");
//		std::exit(1);
//	}
//
//	return reg_label_width_mapping.at(reg_label);
//}
//
//auto Reg::str_of_label(Reg::Label label) -> std::string {
//	for (const auto &[reg_name, reg_label] : str_reg_label_mapping) {
//		if (reg_label != label) continue;
//		return reg_name;
//	}
//
//	fmt::print("'str_reg_label_mapping' is out of sync\n");
//	std::exit(1);
//}
//
//auto Reg::label_of_str(std::string_view reg_name) -> Reg::Label {
//	std::string reg_name_lowercase;
//	reg_name_lowercase.reserve(reg_name.size());
//
//	std::transform(reg_name.begin(), reg_name.end(),
//			reg_name_lowercase.begin(), [](char c) { return std::tolower(c); });
//
//	// Debug
//	if (not str_reg_label_mapping.contains(reg_name_lowercase)) {
//		fmt::print("'{}' does not name a valid register\n", reg_name_lowercase);
//	}
//
//	return str_reg_label_mapping.at(reg_name_lowercase);
//}
//
//auto Mnemonic::spelling_of_str(std::string_view mnemonic_name) -> Mnemonic::Spelling {
//	if (not str_mnemonic_spelling_mapping.contains(mnemonic_name)) {
//		fmt::print("'{}' does not name a valid x86 mnemonic\n", mnemonic_name);
//		std::exit(1);
//	}
//	return str_mnemonic_spelling_mapping.find(mnemonic_name)->second;
//}
//
//auto Mnemonic::str_of_spelling(Mnemonic::Spelling spelling) -> std::string {
//	for (const auto &[mnemonic_name, mnemonic_spelling] : str_mnemonic_spelling_mapping) {
//		if (mnemonic_spelling != spelling) continue;
//		return mnemonic_name;
//	}
//
//	fmt::print("'str_mnemonic_spelling_mapping' is out of sync\n");
//	std::exit(1);
//}
//
//auto mov_reg_reg(ConstRegRef dst, ConstRegRef src) -> ByteVec {
//	u8 opcode = 0x88;
//	ModRmByte mod_rm = {
//		.mod = 0b11,
//		.reg = Reg::index(src),
//		.rm = Reg::index(dst),
//	};
//
//	return { opcode, mod_rm };
//}
