#include "evaluator.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI 3.1415926535

struct evaluator_t {
    calculator_ctx_t* calc_ctx;
};

evaluator_t* evaluator_create(calculator_ctx_t* calc_ctx) {
    evaluator_t* eval = malloc(sizeof(evaluator_t));
    if (eval) {
        eval->calc_ctx = calc_ctx;
    }
    return eval;
}

void evaluator_destroy(evaluator_t* eval) {
    free(eval);
}

static double factorial(double n) {
    // Проверяем, является ли число целым и неотрицательным
    if (n < 0 || n != floor(n)) {
        return NAN;  // Not-a-Number для обозначения ошибки
    }
    if (n == 0 || n == 1) return 1;
    return n * factorial(n - 1);
}

calc_error_t evaluator_evaluate(evaluator_t* eval, ast_node_t* node, double* result) {
    if (!node) return CALC_ERROR_SYNTAX;
    
    switch (node->type) {
        case AST_NUMBER:
            *result = node->value.number;
            return CALC_SUCCESS;
            
        case AST_VARIABLE: {
            double value;
            if (strcmp(node->value.variable, "PI") == 0) {
                *result = PI;
                return CALC_SUCCESS;
            }
            if (calc_get_variable(eval->calc_ctx, node->value.variable, &value) != CALC_SUCCESS) {
                return CALC_ERROR_UNDEFINED_VAR;
            }
            *result = value;
            return CALC_SUCCESS;
        }
        
        case AST_UNARY_OP: {
            double operand;
            calc_error_t err = evaluator_evaluate(eval, node->value.unary_op.operand, &operand);
            if (err != CALC_SUCCESS) return err;
            
            if (strcmp(node->value.unary_op.oper, "-") == 0) {
                *result = -operand;
            } else if (strcmp(node->value.unary_op.oper, "sin") == 0) {
                *result = sin(operand);
            } else if (strcmp(node->value.unary_op.oper, "cos") == 0) {
                *result = cos(operand);
            } else if (strcmp(node->value.unary_op.oper, "!") == 0) {
                *result = factorial(operand);
                // Проверяем результат factorial на NaN
                if (isnan(*result)) {
                    return CALC_ERROR_INVALID_OPERATION;
                }
            } else {
                return CALC_ERROR_INVALID_OPERATION;
            }
            return CALC_SUCCESS;
        }
        
        case AST_BINARY_OP: {
            double left, right;
            calc_error_t err = evaluator_evaluate(eval, node->value.binary_op.left, &left);
            if (err != CALC_SUCCESS) return err;
            
            err = evaluator_evaluate(eval, node->value.binary_op.right, &right);
            if (err != CALC_SUCCESS) return err;
            
            switch (node->value.binary_op.oper) {
                case '+': *result = left + right; break;
                case '-': *result = left - right; break;
                case '*': *result = left * right; break;
                case '/':
                    if (right == 0) return CALC_ERROR_INVALID_OPERATION;
                    *result = left / right;
                    break;
                case '^': *result = pow(left, right); break;
                default: return CALC_ERROR_INVALID_OPERATION;
            }
            return CALC_SUCCESS;
        }
    }
    return CALC_ERROR_SYNTAX;
}