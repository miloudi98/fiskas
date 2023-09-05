#include <filesystem>
#include <execinfo.h>
#include <vector>
#include <string>

#include "base.hh"

auto print_stack_trace() -> void {
	const int max_stack_frames = 128;
	void *buffer[max_stack_frames];

	int stack_frames = backtrace(buffer, max_stack_frames);
	char **strings = backtrace_symbols(buffer, stack_frames);

	if (strings == NULL) {
		fmt::print("Failed to grab the stack trace\n");
		std::exit(1);
	}

	for (i32 i = 0; i < stack_frames; ++i) {
		std::string func = strings[i];
		// Skip debug function calls.
		if (func.find("libasan.so") != std::string::npos) continue;
		if (func.find("print_stack_trace") != std::string::npos) continue;
		if (func.find("fiska_assert_impl") != std::string::npos) continue;

		fmt::print("--> {}\n", std::string(strings[i]));
	}

	free(strings);
}

[[noreturn]] auto ::detail::fiska_assert_impl(
		std::string error_msg,
		std::string file,
		int line,
		std::string helper_msg) -> void
{
	Color C;
	std::string out;

	fs::path project_root_path("/home/hmida/fiska/experimental/assembler");
	fs::path file_path(file);
	fs::path relative_path = fs::relative(file_path, project_root_path);

	out += "====================================================\n";
	out += fmt::format("{}{}Error:{} {}\n",
			C(Colors::Red), C(Colors::Bold), C(Colors::Reset),
			error_msg);

	out += fmt::format("{}{}{}-->{}:{}:{} {}\n",
			C(Colors::Bold), C(Colors::Cyan), C(Colors::Underline),
			relative_path.string(), line, C(Colors::Reset), helper_msg);

	out += "\n";
	fmt::print("{}", out);

	fmt::print("{}{}{}Backtrace:{}\n",C(Colors::Underline),
			C(Colors::Bold), C(Colors::Cyan), C(Colors::Reset));
	print_stack_trace();

	std::exit(1);
}
