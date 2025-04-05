#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

struct lexer_t {
    const char* input;     // Входная строка для анализа
    size_t position;       // Текущая позиция в строке
    size_t length;         // Длина входной строки
};

/**
 * Проверяет, может ли символ быть частью идентификатора
 * (буква, цифра или подчеркивание)
 * 
 * @param c Проверяемый символ
 * @return true если символ может быть частью идентификатора, иначе false
 */
static bool is_identifier_char(char c) {
    return isalnum(c) || c == '_';
}

/**
 * Создает лексический анализатор для заданной входной строки.
 * 
 * @param input Строка с выражением для анализа
 * @return Указатель на созданный лексический анализатор или NULL при ошибке
 */
lexer_t* lexer_create(const char* input) {
    lexer_t* lexer = malloc(sizeof(lexer_t));
    if (lexer) {
        lexer->input = input;
        lexer->position = 0;
        lexer->length = strlen(input);
    }
    return lexer;
}

/**
 * Освобождает ресурсы, занятые лексическим анализатором.
 * 
 * @param lexer Указатель на лексический анализатор
 */
void lexer_destroy(lexer_t* lexer) {
    free(lexer);
}

/**
 * Пропускает пробельные символы в входной строке.
 * Перемещает текущую позицию до первого непробельного символа.
 * 
 * @param lexer Указатель на лексический анализатор
 */
static void skip_whitespace(lexer_t* lexer) {
    while (lexer->position < lexer->length && 
           isspace(lexer->input[lexer->position])) {
        lexer->position++;
    }
}

/**
 * Разбирает числовой литерал (целое или с плавающей точкой).
 * 
 * @param lexer Указатель на лексический анализатор
 * @return Токен с типом TOKEN_NUMBER и значением числа
 */
static token_t parse_number(lexer_t* lexer) {
    token_t token = {.type = TOKEN_NUMBER};
    char* endptr;
    
    // Преобразуем строку в число
    token.value.number = strtod(lexer->input + lexer->position, &endptr);
    
    // Увеличиваем позицию на количество символов, которые были прочитаны
    lexer->position += (endptr - (lexer->input + lexer->position));
    
    return token;
}

/**
 * Разбирает идентификатор или ключевое слово.
 * 
 * @param lexer Указатель на лексический анализатор
 * @return Токен с типом TOKEN_IDENTIFIER и значением идентификатора
 */
static token_t parse_identifier(lexer_t* lexer) {
    token_t token = {.type = TOKEN_IDENTIFIER};
    size_t start = lexer->position;
    
    // Читаем все символы, которые могут быть частью идентификатора
    while (lexer->position < lexer->length && 
           is_identifier_char(lexer->input[lexer->position])) {
        lexer->position++;
    }
    
    // Выделяем память для идентификатора и копируем его
    size_t length = lexer->position - start;
    token.value.identifier = malloc(length + 1);
    strncpy(token.value.identifier, lexer->input + start, length);
    token.value.identifier[length] = '\0';
    
    return token;
}

/**
 * Получает следующий токен из входной строки.
 * 
 * @param lexer Указатель на лексический анализатор
 * @return Следующий токен
 */
token_t lexer_next_token(lexer_t* lexer) {
    skip_whitespace(lexer);
    
    token_t token = {.type = TOKEN_ERROR};
    token.position = lexer->position;
    
    // Проверяем, достигнут ли конец входной строки
    if (lexer->position >= lexer->length) {
        token.type = TOKEN_EOF;
        return token;
    }
    
    char current = lexer->input[lexer->position];
    
    // Разбираем числа
    if (isdigit(current) || current == '.') {
        return parse_number(lexer);
    }
    
    // Разбираем идентификаторы
    if (isalpha(current) || current == '_') {
        return parse_identifier(lexer);
    }
    
    // Разбираем операторы и скобки
    token.type = TOKEN_OPERATOR;
    switch (current) {
        case '+': case '-': case '*': case '/': case '^': case '!':
            token.type = TOKEN_OPERATOR;
            token.value.oper = current;
            break;
        case '(':
            token.type = TOKEN_LPAREN;
            break;
        case ')':
            token.type = TOKEN_RPAREN;
            break;
        case '{':
            token.type = TOKEN_LBRACE;
            break;
        case '}':
            token.type = TOKEN_RBRACE;
            break;
        case '[':
            token.type = TOKEN_LBRACKET;
            break;
        case ']':
            token.type = TOKEN_RBRACKET;
            break;
        default:
            token.type = TOKEN_ERROR;
            return token;
    }
    
    lexer->position++;
    return token;
}

/**
 * Освобождает ресурсы, занятые токеном.
 * Если токен содержит идентификатор, освобождает память, выделенную для него.
 * 
 * @param token Указатель на токен
 */
void token_destroy(token_t* token) {
    if (token && token->type == TOKEN_IDENTIFIER) {
        free(token->value.identifier);
    }
}