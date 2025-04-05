#include "ast.h"
#include <stdlib.h>
#include <string.h>

/**
 * Создает узел AST для числового значения
 * @param value Числовое значение
 * @return Указатель на созданный узел AST или NULL при ошибке выделения памяти
 */
ast_node_t* ast_create_number(double value) {
    ast_node_t* node = malloc(sizeof(ast_node_t));
    if (node) {
        node->type = AST_NUMBER;
        node->value.number = value;
    }
    return node;
}

/**
 * Создает узел AST для переменной
 * @param name Имя переменной
 * @return Указатель на созданный узел AST или NULL при ошибке выделения памяти
 */
ast_node_t* ast_create_variable(const char* name) {
    ast_node_t* node = malloc(sizeof(ast_node_t));
    if (node) {
        node->type = AST_VARIABLE;
        // Создаем копию строки с именем переменной
        node->value.variable = strdup(name);
    }
    return node;
}

/**
 * Создает узел AST для унарной операции
 * @param op Название операции (например, "sin", "cos", "!")
 * @param operand Указатель на узел AST операнда
 * @return Указатель на созданный узел AST или NULL при ошибке выделения памяти
 */
ast_node_t* ast_create_unary_op(const char* op, ast_node_t* operand) {
    ast_node_t* node = malloc(sizeof(ast_node_t));
    if (node) {
        node->type = AST_UNARY_OP;
        // Создаем копию строки с именем оператора
        node->value.unary_op.oper = strdup(op);
        node->value.unary_op.operand = operand;
    }
    return node;
}

/**
 * Создает узел AST для бинарной операции
 * @param op Символ операции (+, -, *, /, ^)
 * @param left Указатель на узел AST левого операнда
 * @param right Указатель на узел AST правого операнда
 * @return Указатель на созданный узел AST или NULL при ошибке выделения памяти
 */
ast_node_t* ast_create_binary_op(char op, ast_node_t* left, ast_node_t* right) {
    ast_node_t* node = malloc(sizeof(ast_node_t));
    if (node) {
        node->type = AST_BINARY_OP;
        node->value.binary_op.oper = op;
        node->value.binary_op.left = left;
        node->value.binary_op.right = right;
    }
    return node;
}

/**
 * Освобождает память, занятую узлом AST и всеми его дочерними узлами
 * Рекурсивно обходит дерево и освобождает память для всех узлов
 * @param node Указатель на узел AST для удаления
 */
void ast_destroy(ast_node_t* node) {
    if (!node) return;
    
    // Обрабатываем различные типы узлов
    switch (node->type) {
        case AST_VARIABLE:
            // Освобождаем память строки с именем переменной
            free(node->value.variable);
            break;
        case AST_UNARY_OP:
            // Освобождаем память строки с именем операции и рекурсивно удаляем операнд
            free(node->value.unary_op.oper);
            ast_destroy(node->value.unary_op.operand);
            break;
        case AST_BINARY_OP:
            // Рекурсивно удаляем левый и правый операнды
            ast_destroy(node->value.binary_op.left);
            ast_destroy(node->value.binary_op.right);
            break;
        default:
            // Для числовых узлов дополнительной очистки не требуется
            break;
    }
    
    // Освобождаем память самого узла
    free(node);
}