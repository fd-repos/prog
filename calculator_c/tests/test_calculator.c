#include "calculator.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#define EPSILON 1e-10
#define PI 3.1415926535

static int double_eq(double a, double b) {
    return fabs(a - b) < EPSILON;
}

static void test_basic_arithmetic(void) {
    calculator_ctx_t* calc = calc_create();
    double result;
    
    assert(calc_evaluate(calc, "2 + 3", &result) == CALC_SUCCESS);
    assert(double_eq(result, 5.0));
    
    assert(calc_evaluate(calc, "5 - 3", &result) == CALC_SUCCESS);
    assert(double_eq(result, 2.0));
    
    assert(calc_evaluate(calc, "4 * 3", &result) == CALC_SUCCESS);
    assert(double_eq(result, 12.0));
    
    assert(calc_evaluate(calc, "10 / 2", &result) == CALC_SUCCESS);
    assert(double_eq(result, 5.0));
    
    assert(calc_evaluate(calc, "2 ^ 3", &result) == CALC_SUCCESS);
    assert(double_eq(result, 8.0));
    
    calc_destroy(calc);
    printf("Basic arithmetic tests passed\n");
}

static void test_operator_precedence(void) {
    calculator_ctx_t* calc = calc_create();
    double result;
    
    assert(calc_evaluate(calc, "2 + 3 * 4", &result) == CALC_SUCCESS);
    assert(double_eq(result, 14.0));
    
    assert(calc_evaluate(calc, "(2 + 3) * 4", &result) == CALC_SUCCESS);
    assert(double_eq(result, 20.0));
    
    assert(calc_evaluate(calc, "2 ^ 3 * 4", &result) == CALC_SUCCESS);
    assert(double_eq(result, 32.0));
    
    calc_destroy(calc);
    printf("Operator precedence tests passed\n");
}

static void test_variables(void) {
    calculator_ctx_t* calc = calc_create();
    double result;
    
    assert(calc_set_variable(calc, "x", 5.0) == CALC_SUCCESS);
    assert(calc_set_variable(calc, "y", 3.0) == CALC_SUCCESS);
    
    assert(calc_evaluate(calc, "x + y", &result) == CALC_SUCCESS);
    assert(double_eq(result, 8.0));
    
    assert(calc_evaluate(calc, "x * y", &result) == CALC_SUCCESS);
    assert(double_eq(result, 15.0));
    
    assert(calc_evaluate(calc, "PI", &result) == CALC_SUCCESS);
    assert(double_eq(result, PI));
    
    assert(calc_evaluate(calc, "z", &result) == CALC_ERROR_UNDEFINED_VAR);
    
    calc_destroy(calc);
    printf("Variable tests passed\n");
}

static void test_unary_operations(void) {
    calculator_ctx_t* calc = calc_create();
    double result;
    
    assert(calc_evaluate(calc, "-5", &result) == CALC_SUCCESS);
    assert(double_eq(result, -5.0));
    
    assert(calc_evaluate(calc, "5!", &result) == CALC_SUCCESS);
    assert(double_eq(result, 120.0));
    
    assert(calc_evaluate(calc, "sin(PI/2)", &result) == CALC_SUCCESS);
    assert(double_eq(result, 1.0));
    
    assert(calc_evaluate(calc, "cos(PI)", &result) == CALC_SUCCESS);
    assert(double_eq(result, -1.0));
    
    calc_destroy(calc);
    printf("Unary operation tests passed\n");
}

static void test_error_handling(void) {
    calculator_ctx_t* calc = calc_create();
    double result;
    
    assert(calc_evaluate(calc, "1/0", &result) == CALC_ERROR_INVALID_OPERATION);
    
    assert(calc_evaluate(calc, "1 + + 2", &result) == CALC_ERROR_SYNTAX);
    
    assert(calc_evaluate(calc, "(1 + 2", &result) == CALC_ERROR_SYNTAX);
    
    assert(calc_evaluate(calc, "(-1)!", &result) == CALC_ERROR_INVALID_OPERATION);
    
    calc_destroy(calc);
    printf("Error handling tests passed\n");
}

static void test_complex_expressions(void) {
    calculator_ctx_t* calc = calc_create();
    double result;
    
    calc_set_variable(calc, "x", PI);
    calc_set_variable(calc, "y", 2.0);
    
    assert(calc_evaluate(calc, "2 + sin(x) / {y + cos(x)} * PI", &result) == CALC_SUCCESS);
    
    assert(calc_evaluate(calc, "2 * (3 + 4 * (5 + 6))", &result) == CALC_SUCCESS);
    assert(double_eq(result, 94.0));
    
    assert(calc_evaluate(calc, "sin(PI/2)^2 + cos(PI/2)^2", &result) == CALC_SUCCESS);
    assert(double_eq(result, 1.0));
    
    calc_destroy(calc);
    printf("Complex expression tests passed\n");
}

int main(void) {
    printf("Running calculator tests...\n\n");
    
    test_basic_arithmetic();
    test_operator_precedence();
    test_variables();
    test_unary_operations();
    test_error_handling();
    test_complex_expressions();
    
    printf("\nAll tests passed successfully!\n");
    return 0;
}