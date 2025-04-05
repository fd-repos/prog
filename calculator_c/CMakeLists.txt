cmake_minimum_required(VERSION 3.10)
project(calculator_c C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_library(calculator
    src/calculator.c
    src/lexer.c
    src/parser.c
    src/ast.c
    src/evaluator.c
)

target_include_directories(calculator
    PUBLIC include
    PRIVATE src
)

find_library(MATH_LIBRARY m)

add_executable(calculator_c examples/main.c)
target_link_libraries(calculator_c PRIVATE calculator ${MATH_LIBRARY})

enable_testing()
add_executable(test_calculator tests/test_calculator.c)
target_link_libraries(test_calculator PRIVATE calculator ${MATH_LIBRARY})
add_test(NAME test_calculator COMMAND test_calculator)