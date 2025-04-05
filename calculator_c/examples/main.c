#include "calculator.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * Выводит информацию о использовании программы.
 * 
 * @param program_name Имя программы
 */
static void print_usage(const char* program_name) {
    printf("Использование: %s \"выражение\" [--var имя=значение ...]\n", program_name);
    printf("Пример: %s \"2 + sin(x) / {y + cos(x)} * PI\" --var x=PI --var y=2\n", program_name);
}

/**
 * Разбирает переменную в формате "имя=значение".
 * Поддерживает специальные значения, такие как PI.
 * 
 * @param calc Указатель на контекст калькулятора
 * @param var_str Строка с объявлением переменной
 * @return Код ошибки: CALC_SUCCESS при успехе, иначе код ошибки
 */
static calc_error_t parse_variable(calculator_ctx_t* calc, const char* var_str) {
    char* eq_pos = strchr(var_str, '=');
    if (!eq_pos) {
        return CALC_ERROR_SYNTAX;
    }

    // Вычисляем длину имени и выделяем буфер
    size_t name_len = eq_pos - var_str;
    char* name = malloc(name_len + 1);
    if (!name) {
        return CALC_ERROR_SYNTAX;
    }
    
    // Копируем и завершаем имя нулевым символом
    strncpy(name, var_str, name_len);
    name[name_len] = '\0';

    // Получаем строку со значением
    const char* value_str = eq_pos + 1;
    double value;

    // Проверяем на специальные значения
    if (strcmp(value_str, "PI") == 0) {
        value = 3.1415926535;
    } else {
        // Преобразуем строку значения в число
        char* endptr;
        value = strtod(value_str, &endptr);
        
        // Проверяем, что преобразование прошло успешно
        if (*endptr != '\0') {
            free(name);
            return CALC_ERROR_SYNTAX;
        }
    }

    // Устанавливаем переменную в контексте калькулятора
    calc_error_t result = calc_set_variable(calc, name, value);
    free(name);
    return result;
}

/**
 * Главная функция программы.
 * Обрабатывает аргументы командной строки, создает контекст калькулятора,
 * устанавливает переменные и вычисляет выражение.
 * 
 * @param argc Количество аргументов командной строки
 * @param argv Массив аргументов командной строки
 * @return Код возврата: 0 при успехе, 1 при ошибке
 */
int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Создаем контекст калькулятора
    calculator_ctx_t* calc = calc_create();
    if (!calc) {
        fprintf(stderr, "Не удалось создать контекст калькулятора\n");
        return 1;
    }
    
    // Разбираем аргументы командной строки
    for (int i = 2; i < argc; i++) {
        if (strncmp(argv[i], "--var", 5) == 0) {
            // Обрабатываем опцию --var
            if (++i >= argc) {
                fprintf(stderr, "Отсутствует значение для опции --var\n");
                calc_destroy(calc);
                return 1;
            }
            
            calc_error_t error = parse_variable(calc, argv[i]);
            if (error != CALC_SUCCESS) {
                fprintf(stderr, "Ошибка установки переменной: %s\n", calc_get_error_message(error));
                calc_destroy(calc);
                return 1;
            }
        } else {
            fprintf(stderr, "Неизвестная опция: %s\n", argv[i]);
            print_usage(argv[0]);
            calc_destroy(calc);
            return 1;
        }
    }
    
    // Evaluate expression
    double result;
    calc_error_t error = calc_evaluate(calc, argv[1], &result);
    
    if (error != CALC_SUCCESS) {
        fprintf(stderr, "Evaluation error: %s\n", calc_get_error_message(error));
        calc_destroy(calc);
        return 1;
    }
    
    printf("%g\n", result);
    
    calc_destroy(calc);
    return 0;
}