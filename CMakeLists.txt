cmake_minimum_required(VERSION 3.0)
project(Phi VERSION 0.1)

set (PhiSrc
	src/stack.c
	src/ast.c
	src/codegen.c
	src/binaryops.c
	src/llvmcontrol.c
	src/main.c
)

# Add Bison and Flex to the project and set their targets
find_package(FLEX REQUIRED)
message(STATUS "Found package Flex.")
find_package(BISON REQUIRED)
message(STATUS "Found package Bison.")

BISON_TARGET(parser src/parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.c
		COMPILE_FLAGS "-d"
		DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/parser.h)
FLEX_TARGET(lexer src/lexer.l ${CMAKE_CURRENT_BINARY_DIR}/lexer.c)
ADD_FLEX_BISON_DEPENDENCY(lexer parser)

# Add llvm to the project and configure the flags
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM version ${LLVM_PACKAGE_VERSION}.")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Set the target and include my own directories
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(src)

add_executable(phi ${PhiSrc} ${BISON_parser_OUTPUTS} ${FLEX_lexer_OUTPUTS})

target_link_libraries(phi LLVM)
target_compile_options(phi PRIVATE -Wall -Wextra -Werror -pedantic -O2)
