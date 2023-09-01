cmake_minimum_required(VERSION 3.20)
project(assembler VERSION 1.0.0 LANGUAGES CXX)

set (CMAKE_EXPORT_COMPILE_COMMANDS ON)
set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# This CMake file only accounts for a GCC compiler. We will add support
# for more compilers in the future.
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message(FATAL_ERROR "Only GCC is supported at the moment.")
endif()

## ============================================================================
##  Install third party libraries
##  Third party libraries installed:
##      1. libfmt
##      2. googletest
## ============================================================================
cmake_policy(SET CMP0135 NEW)

include(FetchContent)
message(STATUS "Downloading and installing libfmt...")
FetchContent_Declare(
	libfmt
	GIT_REPOSITORY "https://github.com/fmtlib/fmt.git"
	GIT_TAG 10.1.0
	SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/fmt"
)
FetchContent_MakeAvailable(libfmt)

message(STATUS "Downloading and installing googletest...")
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/f8d7d77c06936315286eb55f8de22cd23c188571.zip
  SOURCE_DIR "${PROJECT_SOURCE_DIR}/third_party/googletest"
)
FetchContent_MakeAvailable(googletest)

## ============================================================================
## Internal libs
## ============================================================================
file(GLOB_RECURSE library-sources
	"${PROJECT_SOURCE_DIR}/lib/*.cc"
	"${PROJET_SOURCE_DIR}/include/*.hh")

add_library(assembler STATIC ${library-sources})

target_include_directories(assembler PUBLIC "${PROJECT_SOURCE_DIR}/include")
# Include directories of the third_party libraries installed.
target_include_directories(assembler PUBLIC "${libfmt_SOURCE_DIR}/include")

# More compile options.
target_compile_options(assembler PUBLIC 
	    # Add colors to the diagnostics.
		-fdiagnostics-color=always
        ## Warnings.
        -Wall -Wextra     # Enable ‘all’ warnings.
        -Wundef           # Invalid #undef or undefined macro in #if.
        -Wcast-align      # Casting that changes alignment.
        -Wconversion      # Implicit conversions.
        -Wsign-conversion # Implicit sign conversions.
        -Wformat=2        # Stricter format checking.

        ## Disabled warnings.
        -Wno-unused-function
        -Wno-unused-local-typedefs
        -Wno-unused-parameter

        ## NULL Errors.
        -Werror=nonnull # Passing NULL to nonnull parameter.

        ## Memory Errors.
        -Werror=address              # Suspicious use of addresses.
        -Werror=init-self            # Initialization of a variable with itself.
        -Werror=uninitialized

        ## Return type.
        -Werror=return-type

        ## C/C++.
        -Werror=implicit-fallthrough
        -Werror=missing-include-dirs # User-specified include dir does not exist.
        -Werror=pointer-arith        # Disallow void* and function pointer arithmetic.
        -Werror=string-compare       # Nonsensical string comparisons.
        -Werror=switch               # Missing switch cases.
        # -Werror=switch-enum          # Switch on enum (even if there is a default case).
        -Werror=write-strings        # Strings in C should be const char*.

        ## C++.
        -Werror=missing-field-initializers
        -Werror=non-virtual-dtor
        -Werror=pessimizing-move

        -Wlogical-op      # Duplicate or unintended logical operators.
        -Werror=invalid-memory-model # For atomics.
        -Werror=maybe-uninitialized
        -Werror=missing-requires
        -Werror=return-local-addr
)

# Add debug info and sanitizers when needed.
target_compile_options(assembler PUBLIC 
	$<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3 -rdynamic -fsanitize=address>
	$<$<CONFIG:RELEASE>:-O3 -march=native>)
target_link_options(assembler PUBLIC 
	$<$<CONFIG:DEBUG>:-O0 -g3 -ggdb3 -rdynamic -fsanitize=address>
	$<$<CONFIG:RELEASE>:-O3 -march=native>)

# Link libfmt to our library
target_link_libraries(assembler PUBLIC fmt)

## ============================================================================
## Executables 
## ============================================================================
add_executable(assembler_main "${PROJECT_SOURCE_DIR}/src/main.cc")
set_target_properties(assembler_main PROPERTIES OUTPUT_NAME "assembler_main")
target_link_libraries(assembler_main assembler)
