#include "calculator.h"
#include "lexer.h"
#include "parser.h"
#include "evaluator.h"
#include <stdlib.h>
#include <string.h>

#define MAX_VARIABLES 100

struct calculator_ctx_t {
    calc_variable_t variables[MAX_VARIABLES];
    size_t num_variables;
    evaluator_t* evaluator;
};

/**
 * Создает контекст калькулятора.
 * Выделяет память для контекста и инициализирует его.
 * @return Указатель на созданный контекст или NULL при ошибке
 */
calculator_ctx_t* calc_create(void) {
    calculator_ctx_t* ctx = malloc(sizeof(calculator_ctx_t));
    if (ctx) {
        ctx->num_variables = 0;
        ctx->evaluator = evaluator_create(ctx);
        if (!ctx->evaluator) {
            free(ctx);
            return NULL;
        }
    }
    return ctx;
}

/**
 * Освобождает ресурсы, занятые контекстом калькулятора.
 * Освобождает память, выделенную для имен переменных и оценщика выражений.
 * @param ctx Указатель на контекст калькулятора
 */
void calc_destroy(calculator_ctx_t* ctx) {
    if (ctx) {
        for (size_t i = 0; i < ctx->num_variables; i++) {
            free(ctx->variables[i].name);
        }
        evaluator_destroy(ctx->evaluator);
        free(ctx);
    }
}

/**
 * Устанавливает значение переменной в контексте калькулятора.
 * Если переменная уже существует, обновляет ее значение.
 * Если переменная новая, добавляет ее в список переменных.
 * 
 * @param ctx Указатель на контекст калькулятора
 * @param name Имя переменной
 * @param value Значение переменной
 * @return Код ошибки: CALC_SUCCESS при успехе, иначе код ошибки
 */
calc_error_t calc_set_variable(calculator_ctx_t* ctx, const char* name, double value) {
    if (!ctx || !name) return CALC_ERROR_SYNTAX;
    
    // Проверяем, существует ли уже переменная
    for (size_t i = 0; i < ctx->num_variables; i++) {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            ctx->variables[i].value = value;
            return CALC_SUCCESS;
        }
    }
    
    // Добавляем новую переменную
    if (ctx->num_variables >= MAX_VARIABLES) {
        return CALC_ERROR_SYNTAX;
    }
    
    ctx->variables[ctx->num_variables].name = strdup(name);
    ctx->variables[ctx->num_variables].value = value;
    ctx->num_variables++;
    
    return CALC_SUCCESS;
}

/**
 * Получает значение переменной из контекста калькулятора.
 * Ищет переменную по имени и, если найдена, записывает ее значение
 * в указанное место.
 * 
 * @param ctx Указатель на контекст калькулятора
 * @param name Имя переменной
 * @param value Указатель, куда будет записано значение переменной
 * @return Код ошибки: CALC_SUCCESS при успехе, CALC_ERROR_UNDEFINED_VAR если переменная не найдена
 */
calc_error_t calc_get_variable(calculator_ctx_t* ctx, const char* name, double* value) {
    if (!ctx || !name || !value) return CALC_ERROR_SYNTAX;
    
    for (size_t i = 0; i < ctx->num_variables; i++) {
        if (strcmp(ctx->variables[i].name, name) == 0) {
            *value = ctx->variables[i].value;
            return CALC_SUCCESS;
        }
    }
    
    return CALC_ERROR_UNDEFINED_VAR;
}

/**
 * Вычисляет значение выражения.
 * Выполняет лексический анализ, синтаксический разбор и вычисление выражения.
 * 
 * @param ctx Указатель на контекст калькулятора
 * @param expression Строка с выражением для вычисления
 * @param result Указатель, куда будет записан результат вычисления
 * @return Код ошибки: CALC_SUCCESS при успехе, иначе код ошибки
 */
calc_error_t calc_evaluate(calculator_ctx_t* ctx, const char* expression, double* result) {
    if (!ctx || !expression || !result) return CALC_ERROR_SYNTAX;
    
    // Создаем лексический анализатор
    lexer_t* lexer = lexer_create(expression);
    if (!lexer) return CALC_ERROR_SYNTAX;
    
    // Создаем синтаксический анализатор
    parser_t* parser = parser_create(lexer);
    if (!parser) {
        lexer_destroy(lexer);
        return CALC_ERROR_SYNTAX;
    }
    
    // Разбираем выражение в АСД (абстрактное синтаксическое дерево)
    ast_node_t* ast = parser_parse(parser);
    if (!ast) {
        parser_destroy(parser);
        lexer_destroy(lexer);
        return CALC_ERROR_SYNTAX;
    }
    
    // Вычисляем выражение с помощью оценщика
    calc_error_t error = evaluator_evaluate(ctx->evaluator, ast, result);
    
    // Освобождаем ресурсы
    ast_destroy(ast);
    parser_destroy(parser);
    lexer_destroy(lexer);
    
    return error;
}

/**
 * Возвращает текстовое описание кода ошибки.
 * 
 * @param error Код ошибки
 * @return Строка с описанием ошибки
 */
const char* calc_get_error_message(calc_error_t error) {
    switch (error) {
        case CALC_SUCCESS: return "Успешно";
        case CALC_ERROR_SYNTAX: return "Синтаксическая ошибка";
        case CALC_ERROR_UNDEFINED_VAR: return "Неопределенная переменная";
        case CALC_ERROR_INVALID_OPERATION: return "Недопустимая операция";
        default: return "Неизвестная ошибка";
    }
}