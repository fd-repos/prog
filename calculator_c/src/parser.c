#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

struct parser_t {
    lexer_t* lexer;             // Указатель на лексический анализатор
    token_t current_token;      // Текущий обрабатываемый токен
    bool has_error;             // Флаг наличия ошибки
    const char* error_message;  // Сообщение об ошибке
};

/**
 * Возвращает приоритет оператора.
 * Чем выше возвращаемое значение, тем выше приоритет.
 * 
 * @param op Символ оператора
 * @return Числовой приоритет оператора
 */
static int get_operator_precedence(char op) {
    switch (op) {
        case '+': case '-': return 1;  // Сложение и вычитание (низший приоритет)
        case '*': case '/': return 2;  // Умножение и деление
        case '^': return 3;            // Возведение в степень
        case '!': return 4;            // Факториал (высший приоритет)
        default: return 0;
    }
}

/**
 * Создает синтаксический анализатор на основе лексического.
 * 
 * @param lexer Указатель на лексический анализатор
 * @return Указатель на созданный синтаксический анализатор или NULL при ошибке
 */
parser_t* parser_create(lexer_t* lexer) {
    parser_t* parser = malloc(sizeof(parser_t));
    if (parser) {
        parser->lexer = lexer;
        parser->has_error = false;
        parser->error_message = NULL;
        parser->current_token = lexer_next_token(lexer);
    }
    return parser;
}

/**
 * Освобождает ресурсы, занятые синтаксическим анализатором.
 * 
 * @param parser Указатель на синтаксический анализатор
 */
void parser_destroy(parser_t* parser) {
    if (parser) {
        token_destroy(&parser->current_token);
        free(parser);
    }
}

/**
 * Потребляет текущий токен и получает следующий.
 * 
 * @param parser Указатель на синтаксический анализатор
 */
static void advance(parser_t* parser) {
    token_destroy(&parser->current_token);
    parser->current_token = lexer_next_token(parser->lexer);
}

/**
 * Разбирает первичные выражения (числа, переменные, выражения в скобках).
 * 
 * @param parser Указатель на синтаксический анализатор
 * @return Указатель на узел AST или NULL при ошибке
 */
static ast_node_t* parse_primary(parser_t* parser) {
    token_t token = parser->current_token;
    
    switch (token.type) {
        case TOKEN_NUMBER: {
            advance(parser);
            ast_node_t* num = ast_create_number(token.value.number);
            
            // Проверяем наличие постфиксного факториала
            if (parser->current_token.type == TOKEN_OPERATOR && 
                parser->current_token.value.oper == '!') {
                advance(parser);
                return ast_create_unary_op("!", num);
            }
            return num;
        }
        
        case TOKEN_IDENTIFIER: {
            char* name = strdup(token.value.identifier);
            advance(parser);
            
            // Проверяем, является ли идентификатор функцией
            if (parser->current_token.type == TOKEN_LPAREN) {
                advance(parser);
                ast_node_t* arg = parse_expression(parser);
                if (!arg) {
                    free(name);
                    return NULL;
                }
                
                if (parser->current_token.type != TOKEN_RPAREN) {
                    parser->has_error = true;
                    parser->error_message = "Ожидается закрывающая скобка";
                    free(name);
                    ast_destroy(arg);
                    return NULL;
                }
                advance(parser);
                
                ast_node_t* func = ast_create_unary_op(name, arg);
                free(name);
                return func;
            }
            
            ast_node_t* var = ast_create_variable(name);
            free(name);
            
            // Проверяем наличие постфиксного факториала
            if (parser->current_token.type == TOKEN_OPERATOR && 
                parser->current_token.value.oper == '!') {
                advance(parser);
                return ast_create_unary_op("!", var);
            }
            return var;
        }
        
        case TOKEN_LPAREN: case TOKEN_LBRACE: case TOKEN_LBRACKET: {
            advance(parser);
            ast_node_t* expr = parse_expression(parser);
            if (!expr) return NULL;
            
            // Определяем ожидаемый тип закрывающей скобки
            token_type_t expected_close;
            switch (token.type) {
                case TOKEN_LPAREN: expected_close = TOKEN_RPAREN; break;
                case TOKEN_LBRACE: expected_close = TOKEN_RBRACE; break;
                case TOKEN_LBRACKET: expected_close = TOKEN_RBRACKET; break;
                default: expected_close = TOKEN_ERROR;
            }
            
            // Проверяем, что скобки согласованы
            if (parser->current_token.type != expected_close) {
                parser->has_error = true;
                parser->error_message = "Несогласованные скобки";
                ast_destroy(expr);
                return NULL;
            }
            advance(parser);
            
            // Проверяем наличие постфиксного факториала
            if (parser->current_token.type == TOKEN_OPERATOR && 
                parser->current_token.value.oper == '!') {
                advance(parser);
                return ast_create_unary_op("!", expr);
            }
            return expr;
        }
        
        default:
            parser->has_error = true;
            parser->error_message = "Неожиданный токен";
            return NULL;
    }
}

/**
 * Разбирает унарные операции.
 * 
 * @param parser Указатель на синтаксический анализатор
 * @return Указатель на узел AST или NULL при ошибке
 */
static ast_node_t* parse_unary(parser_t* parser) {
    if (parser->current_token.type == TOKEN_OPERATOR) {
        char op = parser->current_token.value.oper;
        if (op == '-' || op == '+') {  // Унарный минус или плюс
            advance(parser);
            ast_node_t* operand = parse_unary(parser);
            if (!operand) return NULL;
            
            if (op == '+') return operand;  // Унарный плюс можно игнорировать
            return ast_create_unary_op("-", operand);
        }
    }
    
    return parse_primary(parser);
}

/**
 * Разбирает бинарные операции с учетом приоритета.
 * Использует алгоритм восхождения по приоритетам.
 * 
 * @param parser Указатель на синтаксический анализатор
 * @param precedence Минимальный приоритет операторов, которые будут обработаны
 * @return Указатель на узел AST или NULL при ошибке
 */
static ast_node_t* parse_binary(parser_t* parser, int precedence) {
    ast_node_t* left = parse_unary(parser);
    if (!left) return NULL;
    
    while (parser->current_token.type == TOKEN_OPERATOR) {
        char op = parser->current_token.value.oper;
        int op_precedence = get_operator_precedence(op);
        
        // Если приоритет оператора не выше текущего минимального, завершаем цикл
        if (op_precedence <= precedence) break;
        
        // Проверяем наличие последовательных операторов (ошибка синтаксиса)
        advance(parser);
        if (parser->current_token.type == TOKEN_OPERATOR && 
            parser->current_token.value.oper != '!' &&    // Разрешаем факториал после оператора
            parser->current_token.value.oper != '-') {    // Разрешаем унарный минус после оператора
            parser->has_error = true;
            parser->error_message = "Последовательные операторы не допускаются";
            ast_destroy(left);
            return NULL;
        }
        
        // Разбираем правую часть выражения с повышенным минимальным приоритетом
        ast_node_t* right = parse_binary(parser, op_precedence);
        if (!right) {
            ast_destroy(left);
            return NULL;
        }
        
        // Создаем узел бинарной операции
        left = ast_create_binary_op(op, left, right);
    }
    
    return left;
}

/**
 * Разбирает выражение верхнего уровня.
 * 
 * @param parser Указатель на синтаксический анализатор
 * @return Указатель на корневой узел AST или NULL при ошибке
 */
static ast_node_t* parse_expression(parser_t* parser) {
    return parse_binary(parser, 0);
}

/**
 * Запускает синтаксический анализ выражения.
 * 
 * @param parser Указатель на синтаксический анализатор
 * @return Указатель на корневой узел AST или NULL при ошибке
 */
ast_node_t* parser_parse(parser_t* parser) {
    ast_node_t* result = parse_expression(parser);
    
    // Проверяем, что после разбора выражения не осталось непрочитанных токенов
    if (result && parser->current_token.type != TOKEN_EOF) {
        parser->has_error = true;
        parser->error_message = "Неожиданные токены после выражения";
        ast_destroy(result);
        return NULL;
    }
    
    return result;
}