cmake_minimum_required(VERSION 3.10)
project(ptxKompilierer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

execute_process(
    COMMAND llvm-config --libs all
    OUTPUT_VARIABLE LLVM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND llvm-config --ldflags
    OUTPUT_VARIABLE LLVM_LDFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND llvm-config --system-libs
    OUTPUT_VARIABLE LLVM_SYSTEM_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

set(PARSER_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.tab.cpp)
set(PARSER_H ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.tab.h)
set(LEXER_CPP ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.cpp)

add_custom_command(
    OUTPUT ${PARSER_CPP} ${PARSER_H}
    COMMAND bison -d -o ${PARSER_CPP} ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
    COMMENT "Generating parser with bison"
)

add_custom_command(
    OUTPUT ${LEXER_CPP}
    COMMAND flex -o ${LEXER_CPP} ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l ${PARSER_H}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src
    COMMENT "Generating lexer with flex"
)

add_custom_target(generate_parser_lexer
    DEPENDS ${PARSER_CPP} ${PARSER_H} ${LEXER_CPP}
)

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ast.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/codegen.cpp
    ${PARSER_CPP}
    ${LEXER_CPP}
)

add_executable(ptx_compiler ${SOURCES})

add_dependencies(ptx_compiler generate_parser_lexer)

set_target_properties(ptx_compiler PROPERTIES
    LINK_FLAGS "${LLVM_LDFLAGS}"
)

target_link_libraries(ptx_compiler ${LLVM_LIBS} ${LLVM_SYSTEM_LIBS})

set_target_properties(ptx_compiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

message(STATUS "LLVM LIBS: ${LLVM_LIBS}")
