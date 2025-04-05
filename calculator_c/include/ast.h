#ifndef AST_H
#define AST_H

/**
 * Перечисление типов узлов абстрактного синтаксического дерева
 */
typedef enum {
    AST_NUMBER,    // Числовая константа
    AST_VARIABLE,  // Переменная
    AST_UNARY_OP,  // Унарная операция (например, sin, cos, факториал)
    AST_BINARY_OP  // Бинарная операция (+, -, *, /, ^)
} ast_node_type_t;

/**
 * Структура узла абстрактного синтаксического дерева (AST)
 * Представляет собой универсальный узел, который может быть одним
 * из четырех типов: число, переменная, унарная операция или бинарная операция
 */
typedef struct ast_node_t {
    ast_node_type_t type;  // Тип узла
    union {
        double number;     // Значение, если узел - число
        char* variable;    // Имя переменной, если узел - переменная
        struct {
            char* oper;    // Имя операции (например, "sin", "cos", "!")
            struct ast_node_t* operand;  // Операнд унарной операции
        } unary_op;
        struct {
            char oper;     // Символ операции (+, -, *, /, ^)
            struct ast_node_t* left;     // Левый операнд
            struct ast_node_t* right;    // Правый операнд
        } binary_op;
    } value;
} ast_node_t;

/**
 * Создает узел AST для числового значения
 * @param value Числовое значение
 * @return Указатель на созданный узел AST или NULL при ошибке
 */
ast_node_t* ast_create_number(double value);

/**
 * Создает узел AST для переменной
 * @param name Имя переменной
 * @return Указатель на созданный узел AST или NULL при ошибке
 */
ast_node_t* ast_create_variable(const char* name);

/**
 * Создает узел AST для унарной операции
 * @param op Название операции (например, "sin", "cos", "!")
 * @param operand Указатель на узел AST операнда
 * @return Указатель на созданный узел AST или NULL при ошибке
 */
ast_node_t* ast_create_unary_op(const char* op, ast_node_t* operand);

/**
 * Создает узел AST для бинарной операции
 * @param op Символ операции (+, -, *, /, ^)
 * @param left Указатель на узел AST левого операнда
 * @param right Указатель на узел AST правого операнда
 * @return Указатель на созданный узел AST или NULL при ошибке
 */
ast_node_t* ast_create_binary_op(char op, ast_node_t* left, ast_node_t* right);

/**
 * Освобождает память, занятую узлом AST и всеми его дочерними узлами
 * @param node Указатель на узел AST для удаления
 */
void ast_destroy(ast_node_t* node);

#endif // AST_H