#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "ast.h"
#include "calculator.h"

/**
 * Структура вычислителя выражений
 * Используем неполный тип для скрытия реализации (паттерн "Непрозрачный указатель")
 */
typedef struct evaluator_t evaluator_t;

/**
 * Создает новый вычислитель выражений
 * @param calc_ctx Указатель на контекст калькулятора, используемый для доступа к переменным и операциям
 * @return Указатель на созданный вычислитель или NULL при ошибке
 */
evaluator_t* evaluator_create(calculator_ctx_t* calc_ctx);

/**
 * Освобождает ресурсы, занятые вычислителем выражений
 * @param eval Указатель на вычислитель выражений
 */
void evaluator_destroy(evaluator_t* eval);

/**
 * Вычисляет значение выражения, представленного в виде абстрактного синтаксического дерева
 * @param eval Указатель на вычислитель выражений
 * @param node Корневой узел AST выражения для вычисления
 * @param result Указатель для записи результата вычисления
 * @return Код ошибки (CALC_SUCCESS при успехе)
 */
calc_error_t evaluator_evaluate(evaluator_t* eval, ast_node_t* node, double* result);

#endif // EVALUATOR_H