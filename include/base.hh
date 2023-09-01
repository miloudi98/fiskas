#ifndef __FISKA_ASSEMBLER_BASE_HH__
#define __FISKA_ASSEMBLER_BASE_HH__

#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <fcntl.h>
#include <filesystem>
#include <fmt/format.h>
#include <ranges>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <utility>

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using f32 = float;
using f64 = double;

using iptr = intptr_t;
using uptr = uintptr_t;
using isz = ptrdiff_t;
using usz = size_t;

using namespace std::string_literals;

namespace chr = std::chrono;
namespace fs = std::filesystem;
namespace vws = std::ranges::views;

// ============================================================================
// Utility functions
// ============================================================================
namespace detail {

template <typename... Args>
    requires(... and std::is_same_v<Args, bool>)
auto any(Args... args) -> bool {
    return (... or args);
}

template <typename T>
static auto extend(std::vector<T> &dst, const std::vector<T> &src) {
	std::copy(src.begin(), src.end(), std::back_inserter(dst));
}

template <typename T, typename ...Option>
concept is_any_of = (... or std::same_as<T, Option>);

template <typename T>
concept UnsignedInt = is_any_of<T, u8, u16, u32, u64>;
} // namespace detail

template <typename Enum>
	requires std::is_enum_v<Enum>
constexpr auto operator+(Enum e) -> std::underlying_type_t<Enum> {
	return std::to_underlying(e);

}

// ============================================================================
// Symbol concatenation
// ============================================================================
#define CAT_IMPL(x, y) x##y
#define CAT(x, y) FISKA_CAT_IMPL(x, y)

// ============================================================================
// Colored output to the terminal
// ============================================================================
enum struct Colors {
    Reset,
    Green,
    Blue,
    Cyan,
    Red,
};

struct Color {
    auto operator()(Colors c) const -> std::string_view {
        switch (c) {
            case Colors::Reset:
                return "\033[m";
            case Colors::Green:
                return "\033[32m";
            case Colors::Blue:
                return "\033[34m";
            case Colors::Cyan:
                return "\033[36m";
            case Colors::Red:
                return "\033[31m";
        }
        __builtin_unreachable();
    }
};

// ============================================================================
// Benchmarking utilities
// ============================================================================
struct Timer {
    std::string desc{};
    Color C{};

    explicit Timer(std::string description) : desc(description) {}

    template <typename Callable>
	auto operator->*(Callable &&cb) -> void {
        fmt::print("{}[==== Timing... =====]{} {}\n", C(Colors::Green), C(Colors::Reset), desc);

        auto start = chr::steady_clock::now();
        cb();
        auto end = chr::steady_clock::now();

        auto duration = chr::duration_cast<chr::duration<double>>(end - start);

        fmt::print("{}[==== Finished. =====]{} {} ({}{}{} s)\n", C(Colors::Green), C(Colors::Reset),
                   desc, C(Colors::Cyan), duration.count(), C(Colors::Reset));
    }
};

#define time_it(...) Timer{fmt::format(__VA_ARGS__)}->*[&]

// ============================================================================
// File utilities
// ============================================================================
struct File {
    static auto write(void *data, u64 size, const fs::path &path) -> void {
        auto file = std::fopen(path.c_str(), "wb");
        // Replace this horrible assertion with proper error messages.
        // Replace this with a proper FISKA_ASSERT();
        assert(file);

        while (1) {
            auto written = std::fwrite(data, 1, size, file);
            if (written == size) {
                break;
            }

            // Replace this horrible assertion with proper error messages.
            assert(written == 1);
            data = (u8 *)data + written;
            size -= written;
        }
        std::fclose(file);
    }

    // Map the file to RAM and copy its content to a new vector of bytes.
    static auto load(const fs::path &path) -> std::vector<u8> {
        int fd = open(path.c_str(), O_RDONLY);
        if (fd < 0) {
            perror("failed to open a file");
            std::exit(1);
        }

        struct stat file_stat {};

        if (fstat(fd, &file_stat) < 0) {
            perror("failed to get file stat");
            std::exit(1);
        }

        void *ptr = mmap(nullptr, usz(file_stat.st_size), PROT_READ, MAP_PRIVATE, fd, 0);
        if (ptr == MAP_FAILED) {
            perror("failed to map the file to AS");
            std::exit(1);
        }

        std::vector<u8> content(usz(file_stat.st_size));
        memcpy(content.data(), ptr, usz(file_stat.st_size));

        close(fd);
        munmap(ptr, static_cast<usz>(file_stat.st_size));

        return content;
    }
};

#endif // __FISKA_UNICODE_PARSING_BASE_HH__
