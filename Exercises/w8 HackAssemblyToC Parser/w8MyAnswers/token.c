#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "token.h"

Token *malloc_token() {
    Token *var = (Token *)malloc(sizeof(Token));
    return var;
}

void free_token(Token *var) {
    // We need to avoid freeing var if it's not actually storing a string.
    if (var->type == IDENTIFIER) {
        free(var->value.str_val);
    }
    free(var);
}

void write_token(Token *token, FILE *output) {
    char token_str[MAX_LINE_LENGTH] = "(Type=";

    char type_str[MAX_LINE_LENGTH] = "";
    sprintf(type_str, "%d", token->type);
    strcat(token_str, type_str);
    strcat(token_str, ", Value=");

    char value_str[MAX_LINE_LENGTH];
    switch(token->type) {
        case KEYWORD: sprintf(value_str, "%d", token->value.key_val); break;
        case INTEGER_LITERAL: sprintf(value_str, "%d", token->value.int_val); break;
        case IDENTIFIER: strcpy(value_str, token->value.str_val); break;
        case SYMBOL: value_str[0] = token->value.char_val; value_str[1] = '\0'; break;
        case NEWLINE: value_str[0] = '\0'; break;
        default: exit(EXIT_FAILURE);
    }
    strcat(token_str, value_str);
    strcat(token_str, ")\n");
    fputs(token_str, output);
}

bool read_token(Token *dest, FILE *input) {
    char line[MAX_LINE_LENGTH];
    if (fgets(line, MAX_LINE_LENGTH, input) == NULL) {
        return false;
    }

    int pos = 0;
    while(line[pos++] != '=');
    int type_start = pos;
    while(line[pos++] != ',');
    int type_length = pos - type_start - 1;
    while(line[pos++] != '=');
    int value_start = pos;
    while(line[pos++] != ')');
    int value_length = pos - value_start - 1;

    char type_str[MAX_LINE_LENGTH];
    strncpy(type_str, line + type_start, type_length);
    type_str[type_length] = '\0';

    char value_str[MAX_LINE_LENGTH];
    strncpy(value_str, line + value_start, value_length);
    value_str[value_length] = '\0';

    dest->type = (TokenType) strtol(type_str,NULL,10);
    switch(dest->type) {
        case KEYWORD: dest->value.key_val = strtol(value_str, NULL, 10); break;
        case INTEGER_LITERAL: dest->value.int_val = strtol(value_str, NULL, 10); break;
        case IDENTIFIER:
            dest->value.str_val = malloc(strlen(value_str)+1);
            strcpy(dest->value.str_val, value_str);
            break;
        case SYMBOL: dest->value.char_val = value_str[0]; break;
        case NEWLINE: break;
        default: exit(EXIT_FAILURE);
    }
    return true;
}