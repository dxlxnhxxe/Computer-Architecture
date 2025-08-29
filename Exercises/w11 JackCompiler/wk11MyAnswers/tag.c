#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tag.h"

const char *TagTypeToString[] = {
        [KEYWORD] = "keyword",
        [SYMBOL] = "symbol",
        [INTEGER_LITERAL] = "integerConstant",
        [STRING_LITERAL] = "stringConstant",
        [IDENTIFIER] = "identifier",
        [NON_TERMINAL] = "nonterminal"
};

const char *NonTerminalToString[] = {
        [NT_CLASS] = "class",
        [NT_CLASS_VAR_DEC] = "classVarDec",
        [NT_SUBROUTINE_DEC] = "subroutineDec",
        [NT_PARAMETER_LIST] = "parameterList",
        [NT_SUBROUTINE_BODY] = "subroutineBody",
        [NT_VAR_DEC] = "varDec",
        [NT_STATEMENTS] = "statements",
        [NT_LET_STATEMENT] = "letStatement",
        [NT_IF_STATEMENT] = "ifStatement",
        [NT_WHILE_STATEMENT] = "whileStatement",
        [NT_DO_STATEMENT] = "doStatement",
        [NT_RETURN_STATEMENT] = "returnStatement",
        [NT_EXPRESSION] = "expression",
        [NT_TERM] = "term",
        [NT_EXPRESSION_LIST] = "expressionList",
        [NT_OP] = "op",
        [NT_UNARY_OP] = "unaryOp",
        [NT_TOKENS] = "tokens",
        [NT_SUBROUTINE_CALL] = "subroutineCall"
};

const char *KeywordToString[] = {
        [KW_CLASS] = "class",
        [KW_CONSTRUCTOR] = "constructor",
        [KW_FUNCTION] = "function",
        [KW_METHOD] = "method",
        [KW_FIELD] = "field",
        [KW_STATIC] = "static",
        [KW_VAR] = "var",
        [KW_INT] = "int",
        [KW_CHAR] = "char",
        [KW_BOOLEAN] = "boolean",
        [KW_VOID] = "void",
        [KW_TRUE] = "true",
        [KW_FALSE] = "false",
        [KW_NULL] = "null",
        [KW_THIS] = "this",
        [KW_LET] = "let",
        [KW_DO] = "do",
        [KW_IF] = "if",
        [KW_ELSE] = "else",
        [KW_WHILE] = "while",
        [KW_RETURN] = "return"
};

Tag *malloc_tag() {
    Tag *var = (Tag *)malloc(sizeof(Tag));
    return var;
}

void free_tag(Tag **var) {
    // We need to avoid freeing var if it's not actually storing a string.
    if ((**var).type == IDENTIFIER || (**var).type == STRING_LITERAL || (**var).type == SYMBOL) {
        free((**var).value.str_val);
    }
    free(*var);
}

// We write non-terminals in the form e.g. <class>\n or </class>\n.
// We write terminals in the form e.g. <keyword> class </keyword>\n or <integerConstant> 423 </integerConstant>\n.
// We keep track of opening and closing tags and indent accordingly.
void write_tag(Tag *var, FILE *output) {
    static int indentation_level = 0;

    if (var->type == NON_TERMINAL && var->close_tag) {
        indentation_level--;
    }

    if (indentation_level > MAX_LINE_LENGTH/4) {
        printf("Maximum indentation level exceeded, something has gone very wrong!");
    }
    char initial_spaces[MAX_LINE_LENGTH];
    for (int i=0; i < indentation_level; i++) {
        initial_spaces[2*i] = ' ';
        initial_spaces[2*i+1] = ' ';
    }
    initial_spaces[2*indentation_level] = '\0';

    char tag_str[MAX_LINE_LENGTH];
    strcat(tag_str, initial_spaces);

    if (var->type == NON_TERMINAL) {
        sprintf(tag_str, "%s<%s%s>\n", initial_spaces, (var->close_tag ? "/" : ""),
                NonTerminalToString[var->value.nt_val]);
        fputs(tag_str, output);
        if (!(var->close_tag)) {
            indentation_level++;
        }
        return;
    }
    sprintf(tag_str, "%s<%s> ", initial_spaces, TagTypeToString[var->type]);

    switch(var->type) {
        case KEYWORD: strcat(tag_str, KeywordToString[var->value.key_val]); break;
        case IDENTIFIER:
        case STRING_LITERAL: strcat(tag_str, var->value.str_val); break;
        case SYMBOL:
            // >, <, " and & are all escaped in XML files
            if (var->value.str_val[0] == '>') {
                strcat(tag_str, "&gt;");
            } else if (var->value.str_val[0] == '<') {
                strcat(tag_str, "&lt;");
            } else if (var->value.str_val[0] == '\"') {
                strcat(tag_str, "&quot;");
            } else if (var->value.str_val[0] == '&') {
                strcat(tag_str, "&amp;");
            } else {
                strcat(tag_str, var->value.str_val);
            } break;
        case INTEGER_LITERAL: {
            char temp[10];
            sprintf(temp, "%d", var->value.int_val);
            strcat(tag_str, temp);
            break;
        } default: printf("Impossible tag enum value %d\n", var->type); exit(EXIT_FAILURE);
    }

    char temp[100];
    sprintf(temp, " </%s>\n", TagTypeToString[var->type]);
    strcat(tag_str, temp);
    fputs(tag_str, output);
}

// If the tag is completed immediately after its contents, e.g. <keyword> class </keyword>, then we read it in as
// an entire token and set value accordingly. Otherwise, e.g. <whileStatement> or </letStatement>, we read it in as
// a single tag of type NON_TERMINAL. We set value.nt_val to be the type of the non-terminal, and we set close_tag to true
// if it's an end tag (i.e. contains a /). Note that in either case we read exactly one line of the input file.
bool read_tag(Tag **var, FILE *input) {
    char line[MAX_LINE_LENGTH];
    if (fgets(line, MAX_LINE_LENGTH, input) == NULL) {
        return false;
    }

    int pos = 0;
    while(line[pos++] != '<');
    if (line[pos] == '/') {
        (*var)->close_tag = true;
        pos++;
    } else {
        (*var)->close_tag = false;
    }

    bool non_terminal = true;
    for (int i = 0; i < NUM_JTTS; i++) {
        if (strncmp(line+pos, TagTypeToString[i], strlen(TagTypeToString[i])) == 0) {
            (*var)->type = i;
            non_terminal = false;
            pos += strlen(TagTypeToString[i]) + 2;
        }
    }

    if (non_terminal) {
        int contents_length = 0;
        while(line[pos + ++contents_length] != '>');

        (*var)->type = NON_TERMINAL;
        for (int i = 0; i < NUM_NTS; i++) {
            int nonterm_length = strlen(NonTerminalToString[i]);
            if (strncmp(line+pos, NonTerminalToString[i], nonterm_length) == 0 && nonterm_length == contents_length) {
                (*var)->value.nt_val = i;
                return true;
            }
        }
        printf("Malformed tag \"%s\".\n", line);
        exit(EXIT_FAILURE);
    }

    int contents_length = 0;
    while(line[pos + ++contents_length] != '<'); // Check for < rather than ' ' because of string literals
    contents_length--;

    switch((*var)->type) {
        case KEYWORD:
            for (int i = 0; i < NUM_KWS; i++) {
                int keyword_length = strlen(KeywordToString[i]);
                if (strncmp(line+pos, KeywordToString[i], keyword_length) == 0 &&
                    keyword_length == contents_length) {
                    (*var)->value.key_val = i;
                    return true;
                }
            }
            printf("Malformed tag \"%s\".\n", line);
            exit(EXIT_FAILURE);
        case INTEGER_LITERAL:
            (*var)->value.int_val = strtol(line+pos, NULL, 10);
            return true;
        case STRING_LITERAL:
        case IDENTIFIER:
            (*var)->value.str_val = malloc((contents_length + 1) * sizeof(char));
            strncpy((*var)->value.str_val, line+pos, contents_length);
            (*var)->value.str_val[contents_length] = '\0';
            return true;
        case SYMBOL:
            (*var)->value.str_val = malloc((contents_length + 1) * sizeof(char));
            // >, <, " and & are all escaped in XML files
            if (strncmp(line+pos, "&gt;", 4) == 0) {
                (*var)->value.str_val[0] = '>';
            } else if (strncmp(line+pos, "&lt;", 4) == 0) {
                (*var)->value.str_val[0] = '<';
            } else if (strncmp(line+pos, "&quot;", 6) == 0) {
                (*var)->value.str_val[0] = '\"';
            } else if (strncmp(line+pos, "&amp;", 5) == 0) {
                (*var)->value.str_val[0] = '&';
            } else {
                (*var)->value.str_val[0] = line[pos];
            }
            (*var)->value.str_val[1] = '\0';
            return true;
        default:
            printf("Impossible tag enum value %d", (*var)->type);
            exit(EXIT_FAILURE);
    }
}

void copy_tag(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    write_tag(*current, output);
    advance_tag(current, lookahead, input);
}

void advance_tag(Tag **current, Tag **lookahead, FILE *input) {
    if (*current != NULL) {
        free_tag(current);
    }
    *current = *lookahead;
    *lookahead = malloc_tag();
    if (!(read_tag(lookahead, input))) {
        free(*lookahead);
        *lookahead = NULL;
    }
}

void write_non_terminal(NonTerminal nt, bool end, FILE *output) {
    Tag temp;
    temp.type = NON_TERMINAL;
    temp.value.nt_val = nt;
    temp.close_tag = end;
    write_tag(&temp, output);
}