#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stdbool.h>

/**
 * Коды ошибок калькулятора
 */
typedef enum {
    CALC_SUCCESS = 0,          // Операция выполнена успешно
    CALC_ERROR_SYNTAX,         // Ошибка синтаксиса в выражении
    CALC_ERROR_UNDEFINED_VAR,  // Использована неопределенная переменная
    CALC_ERROR_INVALID_OPERATION // Недопустимая операция (например, деление на ноль)
} calc_error_t;

/**
 * Структура для хранения переменной и её значения
 */
typedef struct {
    char* name;   // Имя переменной
    double value; // Значение переменной
} calc_variable_t;

/**
 * Контекст калькулятора, содержащий состояние и настройки
 * Используем неполный тип для скрытия реализации (паттерн "Непрозрачный указатель")
 */
typedef struct calculator_ctx_t calculator_ctx_t;

/**
 * Публичный API калькулятора
 */

/**
 * Создает новый контекст калькулятора
 * @return Указатель на созданный контекст или NULL при ошибке
 */
calculator_ctx_t* calc_create(void);

/**
 * Освобождает ресурсы, занятые контекстом калькулятора
 * @param ctx Указатель на контекст калькулятора
 */
void calc_destroy(calculator_ctx_t* ctx);

/**
 * Устанавливает значение переменной в контексте
 * @param ctx Указатель на контекст калькулятора
 * @param name Имя переменной
 * @param value Значение переменной
 * @return Код ошибки (CALC_SUCCESS при успехе)
 */
calc_error_t calc_set_variable(calculator_ctx_t* ctx, const char* name, double value);

/**
 * Получает значение переменной из контекста
 * @param ctx Указатель на контекст калькулятора
 * @param name Имя переменной
 * @param value Указатель для записи значения переменной
 * @return Код ошибки (CALC_SUCCESS при успехе, CALC_ERROR_UNDEFINED_VAR если переменная не найдена)
 */
calc_error_t calc_get_variable(calculator_ctx_t* ctx, const char* name, double* value);

/**
 * Вычисляет математическое выражение
 * @param ctx Указатель на контекст калькулятора
 * @param expression Строка с математическим выражением
 * @param result Указатель для записи результата вычисления
 * @return Код ошибки (CALC_SUCCESS при успехе)
 */
calc_error_t calc_evaluate(calculator_ctx_t* ctx, const char* expression, double* result);

/**
 * Возвращает текстовое описание ошибки по её коду
 * @param error Код ошибки
 * @return Строка с описанием ошибки
 */
const char* calc_get_error_message(calc_error_t error);

/**
 * API для расширения функциональности калькулятора
 */

/**
 * Тип функции для унарных операций (функция одного аргумента)
 */
typedef double (*unary_op_func)(double);

/**
 * Тип функции для бинарных операций (функция двух аргументов)
 */
typedef double (*binary_op_func)(double, double);

/**
 * Регистрирует новую унарную операцию
 * @param ctx Указатель на контекст калькулятора
 * @param name Имя операции (например, "tan" для тангенса)
 * @param func Указатель на функцию, реализующую операцию
 * @return Код ошибки (CALC_SUCCESS при успехе)
 */
calc_error_t calc_register_unary_op(calculator_ctx_t* ctx, const char* name, unary_op_func func);

/**
 * Регистрирует новую бинарную операцию
 * @param ctx Указатель на контекст калькулятора
 * @param name Имя операции (символ операции)
 * @param func Указатель на функцию, реализующую операцию
 * @return Код ошибки (CALC_SUCCESS при успехе)
 */
calc_error_t calc_register_binary_op(calculator_ctx_t* ctx, const char* name, binary_op_func func);

#endif // CALCULATOR_H