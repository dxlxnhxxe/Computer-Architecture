#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "tag.h"
#include "symboltable.h"


// Lexing functions
void lex_file(FILE *input, FILE *output);
void lex_line(const char *line, FILE *output);
int lex_token(Tag *dest, const char *line);

// Parsing functions
void parse_file(FILE *input, FILE *output);
void parse_class(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_class_var_dec(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_sub_dec(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_parameter_list(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_sub_body(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_var_dec(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_statements(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_let_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_if_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_while_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_do_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_return_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_expression(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_sub_call(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_term(Tag **current, Tag **lookahead, FILE *input, FILE *output);
void parse_expression_list(Tag **current, Tag **lookahead, FILE *input, FILE *output);

// Holds pointers to all the data we'll need during the compilation process to save us from having to pass seven
// arguments around in every single function call.
struct CompileData {
    Tag *current;
    Tag *lookahead;
    SymbolTable *class_table;
    SymbolTable *subroutine_table;
    char *class_name;
    FILE *input;
    FILE *output;
}; typedef struct CompileData CompileData;

void compile_file(FILE *input, FILE *output);
void compile_class(CompileData *data);
void compile_class_var_dec(CompileData *data);
void compile_sub_dec(CompileData *data);
void compile_parameter_list(CompileData *data);
void compile_sub_body(CompileData *data, Keyword sub_kind, char *sub_name);
void compile_var_dec(CompileData *data);
void compile_statements(CompileData *data);
void compile_let_statement(CompileData *data);
void compile_if_statement(CompileData *data);
void compile_while_statement(CompileData *data);
void compile_do_statement(CompileData *data);
void compile_return_statement(CompileData *data);
void compile_expression(CompileData *data);
void compile_sub_call(CompileData *data);
void compile_term(CompileData *data);
int compile_expression_list(CompileData *data);
char *type_to_string(Tag *current);
void var_to_string(TableEntry *var_entry, char *buffer);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Please supply two arguments: an input .jack file, and an output .vm file.");
        exit(EXIT_FAILURE);
    }
    char *input_name = argv[1];
    char *output_name = argv[2];

    FILE *input = fopen(input_name, "r");
    if (input == NULL) {
        printf("Error opening input file.\n");
        exit(EXIT_FAILURE);
    }
    FILE *lex_output = fopen("lex_out.xml", "w");
    if (lex_output == NULL) {
        printf("Error opening lex_out.xml for writing.\n");
        exit(EXIT_FAILURE);
    }
    lex_file(input, lex_output);
    fclose(input);
    fclose(lex_output);

    FILE *lex_input = fopen("lex_out.xml", "r");
    if (lex_input == NULL) {
        printf("Error opening lex_out.xml for reading.\n");
        exit(EXIT_FAILURE);
    }
    FILE *parse_output = fopen("parse_out.xml", "w");
    if (parse_output == NULL) {
        printf("Error opening parse_out.xml for writing.\n");
        exit(EXIT_FAILURE);
    }
    parse_file(lex_input, parse_output);
    fclose(lex_input);
    fclose(parse_output);

    FILE *parse_input = fopen("parse_out.xml", "r");
    if (parse_input == NULL) {
        printf("Error opening parse_out.xml for reading.\n");
        exit(EXIT_FAILURE);
    }
    FILE *output = fopen(output_name, "w");
    if (output == NULL) {
        printf("Error opening output file.");
        exit(EXIT_FAILURE);
    }

    // Debugging help
    setbuf(output, NULL);

    compile_file(parse_input, output);
    fclose(parse_input);
    fclose(output);

    return EXIT_SUCCESS;
}

// Outputs a tokenised version of input to output.
void lex_file(FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    write_non_terminal(NT_TOKENS, false, output);
    while (fgets(line, MAX_LINE_LENGTH, input) != NULL) {
        lex_line(line, output);
    }
    write_non_terminal(NT_TOKENS, true, output);
}

// Reads the next line from input, tokenises it, and writes the resulting tokens to output.
void lex_line(const char *line, FILE *output) {
    // This will be true if we are currently inside a comment of the form /*...*/ (which may have started on a
    // previous line).
    static bool inside_comment = false;

    int pos = 0;
    // In each iteration of this loop, everything up to line[pos] has been lexed.
    // NB newlines aren't tokens in Jack, so no need to care about \n versus \r\n.
    while (line[pos] != '\n' && line[pos] != '\0') {
        // Ignore all whitespace.
        if (line[pos] == ' ' || line[pos] == '\t') {
            pos++;
            continue;
        }

        // If we are inside a comment, ignore everything until the end of the comment.
        if (inside_comment) {
            if (line[pos] == '*' && line[pos+1] == '/') {
                pos++;
                inside_comment = false;
            }
            pos++;
            continue;
        }

        // Ignore all comments.
        if (line[pos] == '/' && line[pos+1] == '/') {
            return;
        } else if (line[pos] == '/' && line[pos+1] == '*') {
            // Note the comment may end on this line
            inside_comment = true;
            pos += 2;
            continue;
        }

        // Otherwise, we have at least one token, so lex it.
        Tag *next = malloc_tag();
        pos += lex_token(next, line + pos);
        write_tag(next, output);
        free_tag(&next);
    }
}

// Reads the next token from a non-empty, non-label, non-comment line into dest, then returns the number of characters
// in that token.
int lex_token(Tag *dest, const char *line) {
    int length;

    if (line[0] >= '0' && line[0] <= '9') {
        // Identifiers can't start with integers, so this must be an integer literal
        dest->type = INTEGER_LITERAL;
        char literal[MAX_LINE_LENGTH];
        for (length = 0; line[length] >= '0' && line[length] <= '9'; length++) {
            literal[length] = line[length];
        }
        literal[length] = '\0';
        dest->value.int_val = strtol(literal, NULL, 10);
    } else if (line[0] == '\"') {
        // Identifiers can't start with quotes, so this must be a string literal
        dest->type = STRING_LITERAL;
        for (length = 1; line[length] != '\"'; length++);
        length++;
        dest->value.str_val = (char *) malloc(length-1);
        strncpy(dest->value.str_val, line+1, length-2);
        dest->value.str_val[length-2] = '\0'; // Strncpy doesn't null-terminate.
    } else if (line[0] == '{' || line[0] == '}' || line[0] == '(' || line[0] == ')' || line[0] == '[' ||
               line[0] == ']' || line[0] == '.' || line[0] == ',' || line[0] == ';' || line[0] == '+' ||
               line[0] == '-' || line[0] == '*' || line[0] == '/' || line[0] == '&' || line[0] == '|' ||
               line[0] == '<' || line[0] == '=' || line[0] == '>' || line[0] == '~'){
        dest->type = SYMBOL;
        length = 1;
        dest->value.str_val = (char *)(malloc(2));
        dest->value.str_val[0] = line[0];
        dest->value.str_val[1] = '\0';
    } else { // We either have an identifier or a keyword.
        // Either way, it keeps going until a character other than a letter, a digit, or an underscore.
        length = 0;
        while(isalpha(line[length]) || isdigit(line[length]) || line[length] == '_'){
            length++;
        }

        dest->type = KEYWORD; // Default
        if (strncmp(line, "class", length) == 0 && length == 5) {
            dest->value.key_val = KW_CLASS;
        } else if (strncmp(line, "constructor", length) == 0 && length == 11) {
            dest->value.key_val = KW_CONSTRUCTOR;
        } else if (strncmp(line, "function", length) == 0 && length == 8) {
            dest->value.key_val = KW_FUNCTION;
        } else if (strncmp(line, "method", length) == 0 && length == 6) {
            dest->value.key_val = KW_METHOD;
        } else if (strncmp(line, "field", length) == 0 && length == 5) {
            dest->value.key_val = KW_FIELD;
        } else if (strncmp(line, "static", length) == 0 && length == 6) {
            dest->value.key_val = KW_STATIC;
        } else if (strncmp(line, "var", length) == 0 && length == 3) {
            dest->value.key_val = KW_VAR;
        } else if (strncmp(line, "int", length) == 0 && length == 3) {
            dest->value.key_val = KW_INT;
        } else if (strncmp(line, "char", length) == 0 && length == 4) {
            dest->value.key_val = KW_CHAR;
        } else if (strncmp(line, "boolean", length) == 0 && length == 7) {
            dest->value.key_val = KW_BOOLEAN;
        } else if (strncmp(line, "void", length) == 0 && length == 4) {
            dest->value.key_val = KW_VOID;
        } else if (strncmp(line, "true", length) == 0 && length == 4) {
            dest->value.key_val = KW_TRUE;
        } else if (strncmp(line, "false", length) == 0 && length == 5) {
            dest->value.key_val = KW_FALSE;
        } else if (strncmp(line, "null", length) == 0 && length == 4) {
            dest->value.key_val = KW_NULL;
        } else if (strncmp(line, "this", length) == 0 && length == 4) {
            dest->value.key_val = KW_THIS;
        } else if (strncmp(line, "let", length) == 0 && length == 3) {
            dest->value.key_val = KW_LET;
        } else if (strncmp(line, "do", length) == 0 && length == 2) {
            dest->value.key_val = KW_DO;
        } else if (strncmp(line, "if", length) == 0 && length == 2) {
            dest->value.key_val = KW_IF;
        } else if (strncmp(line, "else", length) == 0 && length == 4) {
            dest->value.key_val = KW_ELSE;
        } else if (strncmp(line, "while", length) == 0 && length == 5) {
            dest->value.key_val = KW_WHILE;
        } else if (strncmp(line, "return", length) == 0 && length == 6) {
            dest->value.key_val = KW_RETURN;
        } else { // We've now ruled out all the keywords, so it must be an identifier.
            dest->type = IDENTIFIER;
            dest->value.str_val = (char *)malloc(length+1);
            strncpy(dest->value.str_val, line, length);
            dest->value.str_val[length] = '\0'; // Strncpy doesn't null-terminate.
        }
    }
    return length;
}

// Input should be a tokenised file. Writes a parse tree in XML form to output.
void parse_file(FILE *input, FILE *output) {
    Tag **lookahead = malloc(sizeof(Tag *));
    *lookahead = NULL;
    Tag **current = malloc(sizeof(Tag *));
    *current = NULL;
    advance_tag(current, lookahead, input);
    advance_tag(current, lookahead, input);
    // Now *current points to the first tag in the file, and *lookahead points to the second tag.
    // Note that copy_tag and advance_tag handle malloc'ing and free'ing for us.

    // This tag is in there purely to conform to the XML standard --- see the lab sheet.
    if ((*current)->type != NON_TERMINAL || (*current)->value.nt_val != NT_TOKENS) {
        printf("Error: Token list should start with <tokens>.\n");
        exit(EXIT_FAILURE);
    }
    advance_tag(current, lookahead, input);

    // Now we should be at the class declaration, which we parse in full.
    if ((*current)->type != KEYWORD || (*current)->value.nt_val != KW_CLASS) {
        printf("Error: All Jack files should consist of a single class.\n");
        exit(EXIT_FAILURE);
    }
    parse_class(current, lookahead, input, output);

    // Now our current tag should be the </tokens> tag at the end of the file, and lookahead should be null.
    if ((*lookahead) != NULL) {
        printf("Error: All Jack files should consist of a single class.\n");
        exit(EXIT_FAILURE);
    } if ((*current)->type != NON_TERMINAL || (*current)->value.nt_val != NT_TOKENS) {
        printf("Error: Token list should end with </tokens>.\n");
        exit(EXIT_FAILURE);
    }
    advance_tag(current, lookahead, input);
    // Current and lookahead are already freed.
}

// The general "contract" of a parse_*** function should be: If e.g. parse_if_statement is called, then "current" points
// to the first token of an <ifStatement> non-terminal, and "lookahead" points to the token after "current". On function
// return, "current" will point to the token after the last token in that <ifStatement> (i.e. after the ';') and
// "lookahead" will once again point to the token after "current".
// This is what careful error-checking looks like.
// Children in AST: class name identifier, <classVarDec>, <subroutineDec>.
void parse_class(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax is 'class', identifier, '{', {<classVarDec>}, {<subroutineDec>}, '}'
    write_non_terminal(NT_CLASS, false, output);
    advance_tag(current, lookahead, input);

    if ((*current)->type != IDENTIFIER) {
        printf("Error: <class> expected identifier token.\n");
        exit(EXIT_FAILURE);
    }
    copy_tag(current, lookahead, input, output);

    if ((*current)->type != SYMBOL || strcmp((*current)->value.str_val, "{") != 0) {
        printf("Error: <class> expected '{' token.\n");
        exit(EXIT_FAILURE);
    }
    advance_tag(current, lookahead, input);

    bool var_dec_over = false;
    while (true) {
        // If we see a } then the class is over, close the tag and return.
        if ((*current)->type == SYMBOL && strcmp((*current)->value.str_val, "}") == 0) {
            advance_tag(current, lookahead, input);
            write_non_terminal(NT_CLASS, true, output);
            return;
        }

        // A 'static' or a 'field' means this is a variable declaration. A 'constructor', 'function' or 'method' means
        // this is a subroutine declaration. Anything else is unexpected.
        if ((*current)->type == KEYWORD && ((*current)->value.key_val == KW_STATIC || (*current)->value.key_val == KW_FIELD)) {
            if (var_dec_over) {
                printf("Error: All variables must occur at start of class declaration.\n");
                exit(EXIT_FAILURE);
            }
            parse_class_var_dec(current, lookahead, input, output);
        } else if ((*current)->type == KEYWORD && ((*current)->value.key_val == KW_CONSTRUCTOR ||
                        (*current)->value.key_val == KW_METHOD || (*current)->value.key_val == KW_FUNCTION)) {
            var_dec_over = true;
            parse_sub_dec(current, lookahead, input, output);
        } else {
            printf("Error: Unexpected token in <class>.\n");
            exit(EXIT_FAILURE);
        }

        if (current == NULL) {
            printf("Error: <class> expected '}' token.\n");
            exit(EXIT_FAILURE);
        }
    }
}

// This is what a lack of error-checking looks like. It's much easier!
// Children in AST: kind keyword (static/field), type keyword, list of identifiers.
void parse_class_var_dec(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: ('static' | 'field'), ('int' | 'char' | 'boolean' | identifier), identifier, {',', identifier}*, ';'
    write_non_terminal(NT_CLASS_VAR_DEC, false, output);

    copy_tag(current, lookahead, input, output);
    copy_tag(current, lookahead, input, output);
    copy_tag(current, lookahead, input, output);

    while ((*current)->type != SYMBOL || strcmp((*current)->value.str_val, ";") != 0) {
        advance_tag(current, lookahead, input);
        copy_tag(current, lookahead, input, output);
    }
    advance_tag(current, lookahead, input);

    write_non_terminal(NT_CLASS_VAR_DEC, true, output);
}

// Children in AST: subroutine keyword, return type keyword, subroutine name, <parameterList>, <subroutineBody>.
void parse_sub_dec(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: ('constructor' | 'method' | 'function'), ('int' | 'char' | 'boolean' | 'void' | identifier), identifier,
    // '(', <parameterList>, ')', <subroutineBody>.
    write_non_terminal(NT_SUBROUTINE_DEC, false, output);

    copy_tag(current, lookahead, input, output);
    copy_tag(current, lookahead, input, output);
    copy_tag(current, lookahead, input, output);
    advance_tag(current, lookahead, input);

    parse_parameter_list(current, lookahead, input, output);

    advance_tag(current, lookahead, input);
    parse_sub_body(current, lookahead, input, output);

    write_non_terminal(NT_SUBROUTINE_DEC, true, output);
}

// Children in AST: {<type>, identifier}.
void parse_parameter_list(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    write_non_terminal(NT_PARAMETER_LIST, false, output);

    if ((*current)->type != SYMBOL || (*current)->value.str_val[0] != ')') {
        copy_tag(current, lookahead, input, output);
        copy_tag(current, lookahead, input, output);
    }
    while((*current)->type != SYMBOL || (*current)->value.str_val[0] != ')') {
        advance_tag(current, lookahead, input);
        copy_tag(current, lookahead, input, output);
        copy_tag(current, lookahead, input, output);
    }

    write_non_terminal(NT_PARAMETER_LIST, true, output);
}

// Children in AST: <varDec>s, <statements>.
void parse_sub_body(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: '{', {varDec}, <statements>, '}'
    write_non_terminal(NT_SUBROUTINE_BODY, false, output);

    advance_tag(current, lookahead, input);
    while ((*current)->value.key_val == KW_VAR) {
        parse_var_dec(current, lookahead, input, output);
    }
    parse_statements(current, lookahead, input, output);
    advance_tag(current, lookahead, input);

    write_non_terminal(NT_SUBROUTINE_BODY, true, output);
}

// Children in AST: variable keyword, type keyword, list of variable names.
void parse_var_dec(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: 'var', ('int' | 'char' | 'boolean' | identifier), identifier, {',', identifier}, ';'
    write_non_terminal(NT_VAR_DEC, false, output);
    advance_tag(current, lookahead, input);
    copy_tag(current, lookahead, input, output);
    copy_tag(current, lookahead, input, output);
    while(strcmp((*current)->value.str_val, ";") != 0) {
        advance_tag(current, lookahead, input);
        copy_tag(current, lookahead, input, output);
    }
    advance_tag(current, lookahead, input);
    write_non_terminal(NT_VAR_DEC, true, output);
}

// Children in AST: As in CST.
void parse_statements(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: {<letStatement> | <ifStatement> | <whileStatement> | <doStatement> | <returnStatement>}
    write_non_terminal(NT_STATEMENTS, false, output);

    while (true) {
        switch ((*current)->value.key_val) {
            case KW_LET: parse_let_statement(current, lookahead, input, output); break;
            case KW_IF: parse_if_statement(current, lookahead, input, output); break;
            case KW_WHILE: parse_while_statement(current, lookahead, input, output); break;
            case KW_DO: parse_do_statement(current, lookahead, input, output); break;
            case KW_RETURN: parse_return_statement(current, lookahead, input, output); break;
            default: write_non_terminal(NT_STATEMENTS, true, output); return;
        }
    }
}

// Children in AST: destination variable, [<expression>] if present, right-hand side <expression>.
void parse_let_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: 'let', identifier, ['[', <expression>, ']'], '=', <expression>, ';'
    write_non_terminal(NT_LET_STATEMENT, false, output);
    advance_tag(current, lookahead, input);
    copy_tag(current, lookahead, input, output);

    if (strcmp((*current)->value.str_val, "[") == 0) {
        copy_tag(current, lookahead, input, output);
        parse_expression(current, lookahead, input, output);
        copy_tag(current, lookahead, input, output);
    }

    advance_tag(current, lookahead, input);
    parse_expression(current, lookahead, input, output);
    advance_tag(current, lookahead, input);

    write_non_terminal(NT_LET_STATEMENT, true, output);
}

// Children in AST: Condition <expression>, <statements> in if clause, <statements> in else clause (if present)
void parse_if_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: 'if', '(', <expression>, ')', '{', <statements>, '}', ['else', '{', <statements>, '}']
    write_non_terminal(NT_IF_STATEMENT, false, output);

    advance_tag(current, lookahead, input);
    advance_tag(current, lookahead, input);
    parse_expression(current, lookahead, input, output);
    advance_tag(current, lookahead, input);
    advance_tag(current, lookahead, input);
    parse_statements(current, lookahead, input, output);
    advance_tag(current, lookahead, input);

    if ((*current)->type == KEYWORD && (*current)->value.key_val == KW_ELSE) {
        advance_tag(current, lookahead, input);
        advance_tag(current, lookahead, input);
        parse_statements(current, lookahead, input, output);
        advance_tag(current, lookahead, input);
    }

    write_non_terminal(NT_IF_STATEMENT, true, output);
}

// Children in AST: Condition <expression>, <statements> in loop body.
void parse_while_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: 'while', '(', <expression>, ')', '{', <statements>, '}'
    write_non_terminal(NT_WHILE_STATEMENT, false, output);

    advance_tag(current, lookahead, input);
    advance_tag(current, lookahead, input);
    parse_expression(current, lookahead, input, output);
    advance_tag(current, lookahead, input);
    advance_tag(current, lookahead, input);
    parse_statements(current, lookahead, input, output);
    advance_tag(current, lookahead, input);

    write_non_terminal(NT_WHILE_STATEMENT, true, output);
}

// Children in AST: Just the subroutine call.
void parse_do_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: 'do', <expression>, ';'
    write_non_terminal(NT_DO_STATEMENT, false, output);

    advance_tag(current, lookahead, input);
    parse_sub_call(current, lookahead, input, output);
    advance_tag(current, lookahead, input);

    write_non_terminal(NT_DO_STATEMENT, true, output);
}

// Children in AST: <expression> to return (if present).
void parse_return_statement(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: 'return', [<expression>], ';'
    write_non_terminal(NT_RETURN_STATEMENT, false, output);

    advance_tag(current, lookahead, input);
    if ((*current)->type != SYMBOL || strcmp((*current)->value.str_val, ";") != 0) {
        parse_expression(current, lookahead, input, output);
    }
    advance_tag(current, lookahead, input);

    write_non_terminal(NT_RETURN_STATEMENT, true, output);
}

// Children in AST: As in original, <term>, {<op>, <term>}.
void parse_expression(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: <term>, {<op>, <term>}
    // where <op> ::= '+' | '-' | '*' | '/' | '&' | '|' | '<' | '>' | '='
    write_non_terminal(NT_EXPRESSION, false, output);

    parse_term(current, lookahead, input, output);
    while ((*current)->type == SYMBOL && ((*current)->value.str_val[0] == '+' || (*current)->value.str_val[0] == '-' ||
            (*current)->value.str_val[0] == '*' || (*current)->value.str_val[0] == '/' || (*current)->value.str_val[0] == '&' ||
            (*current)->value.str_val[0] == '|' || (*current)->value.str_val[0] == '<' || (*current)->value.str_val[0] == '>' ||
            (*current)->value.str_val[0] == '=')) {
        copy_tag(current, lookahead, input, output);
        parse_term(current, lookahead, input, output);
    }

    write_non_terminal(NT_EXPRESSION, true, output);
}

// Children in AST: identifier, ['.', identifier], <expressionList>
void parse_sub_call(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: identifier, ['.', identifier], '(', <expressionList>, ')'
    write_non_terminal(NT_SUBROUTINE_CALL, false, output);

    copy_tag(current, lookahead, input, output);
    if ((*current)->type == SYMBOL && (*current)->value.str_val[0] == '.') {
        copy_tag(current, lookahead, input, output);
        copy_tag(current, lookahead, input, output);
    }
    advance_tag(current, lookahead, input);
    parse_expression_list(current, lookahead, input, output);
    advance_tag(current, lookahead, input);

    write_non_terminal(NT_SUBROUTINE_CALL, true, output);
}

// Children in AST: Everything except ()s around an <expression>.
void parse_term(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: integerLiteral | stringLiteral | 'true' | 'false' | 'null' | 'this' |
    //         identifier, ['[', <expression>, ']'] |
    //         '(', <expression>, ')' |
    //         ('-' | '~'), <term> |
    //         <subroutineCall>
    // NOTE: <subroutineCall> starts with an identifier. We need to use lookahead to distinguish between a
    // <subroutineCall> and a terminal identifier.
    write_non_terminal(NT_TERM, false, output);

    if ((*current)->type == IDENTIFIER) {
        // Subroutine call case
        if ((*lookahead)->type == SYMBOL &&
            ((*lookahead)->value.str_val[0] == '.' || (*lookahead)->value.str_val[0] == '(')) {
            parse_sub_call(current, lookahead, input, output);
        } else {
            // Otherwise we have identifier and maybe '[', <expression>, ']'.
            copy_tag(current, lookahead, input, output);
            if ((*current)->type == SYMBOL && (*current)->value.str_val[0] == '[') {
                copy_tag(current, lookahead, input, output);
                parse_expression(current, lookahead, input, output);
                copy_tag(current, lookahead, input, output);
            }
        }
    } else if ((*current)->type == INTEGER_LITERAL || (*current)->type == STRING_LITERAL || (*current)->type == KEYWORD) {
        // Keyword cases are 'true', 'false', 'null' and 'this'.
        copy_tag(current, lookahead, input, output);
    } else {
        // We have either '(', <expression>, ')' or ('-' | '~'), <term>
        if ((*current)->value.str_val[0] == '(') {
            advance_tag(current, lookahead, input);
            parse_expression(current, lookahead, input, output);
            advance_tag(current, lookahead, input);
        } else {
            copy_tag(current, lookahead, input, output);
            parse_term(current, lookahead, input, output);
        }
    }

    write_non_terminal(NT_TERM, true, output);
}

// Children in AST: Just the <expression>s.
void parse_expression_list(Tag **current, Tag **lookahead, FILE *input, FILE *output) {
    // Syntax: [<expression>, {',', <expression>}]
    write_non_terminal(NT_EXPRESSION_LIST, false, output);

    // Note: current will be ')' if and only if the <expression_list> is empty.
    if ((*current)->type != SYMBOL || (*current)->value.str_val[0] != ')') {
        parse_expression(current, lookahead, input, output);
        while((*current)->type == SYMBOL && (*current)->value.str_val[0] == ',') {
            advance_tag(current, lookahead, input);
            parse_expression(current, lookahead, input, output);
        }
    }

    write_non_terminal(NT_EXPRESSION_LIST, true, output);
}

void compile_file(FILE *input, FILE *output) {
    CompileData *data = malloc(sizeof(CompileData));
    data->input = input;
    data->output = output;
    data->class_name = NULL;

    data->class_table = NULL;
    data->subroutine_table = NULL;

    data->current = NULL;
    data->lookahead = NULL;

    advance_tag(&(data->current), &(data->lookahead), input);
    advance_tag(&(data->current), &(data->lookahead), input);

    compile_class(data);

    // *Lookahead, *current, *class and *subroutine will be NULL again at this point, no need to free them.
    free(data);
}

// Outputs all VM code for the given <class> with no side effects.
void compile_class(CompileData *data) {
    // AST format: class name identifier, <classVarDec>s, <subroutineDec>s.
    // Skip open tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Store class name.
    char *current_class_name = data->current->value.str_val;
    data->class_name = malloc(strlen(current_class_name)+1);
    strcpy(data->class_name, current_class_name);
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Create class_table symbol table from <classVarDecs>.
    data->class_table = malloc_table();
    while (data->current->value.nt_val == NT_CLASS_VAR_DEC) {
        compile_class_var_dec(data);
    }

    // Compile all subroutines using class_table symbol table.
    while (data->current->value.nt_val == NT_SUBROUTINE_DEC) {
        compile_sub_dec(data);
    }

    // Cleanup, skip closing tag.
	free(data->class_name);
    free_table(data->class_table);
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Adds the variables from the given <classVarDec> to the class_table symbol table.
void compile_class_var_dec(CompileData *data) {
    // AST format: variable keyword, type keyword, list of identifiers.
    // Skip open tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Store variable kind.
    VariableKind kind = (data->current->value.key_val == KW_FIELD) ? VK_FIELD : VK_STATIC;
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Need to copy variable type as otherwise it will be freed on calling advance_tag again.
    char *type = type_to_string(data->current);
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Add variables to symbol table.
    while(!(data->current->close_tag)) {
        add_to_table(data->class_table, data->current->value.str_val, type, kind);
        advance_tag(&(data->current), &(data->lookahead), data->input);
    }

    // Cleanup, skip closing tag.
    free(type);
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <subroutineDec> with no side effects.
void compile_sub_dec(CompileData *data) {
    // AST format: subroutine keyword, return type keyword, name, <parameterList>, <subroutineBody>.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Store subroutine keyword for later.
    Keyword sub_kind = data->current->value.key_val;
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Return type is unused (except in type-checking which we don't implement). Skip it.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Copy subroutine name for later.
    char *sub_name = malloc(strlen(data->current->value.str_val)+1);
    strcpy(sub_name, data->current->value.str_val);
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Create subroutine symbol table and add all given arguments to it. Methods are passed this as argument 0 in
    // addition to those listed in the <parameterList>.
    data->subroutine_table = malloc_table();
    if (sub_kind == KW_METHOD) {
        add_to_table(data->subroutine_table, "this", data->class_name, VK_ARGUMENT);
    }
    compile_parameter_list(data);

    // Compile the subroutine body.
    compile_sub_body(data, sub_kind, sub_name);

    // Cleanup, skip closing tag.
    free_table(data->subroutine_table);
    free(sub_name);
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <subBody>, populating the subroutine's symbol table in the process.
void compile_sub_body(CompileData *data, Keyword sub_kind, char *sub_name) {
    // AST format: <varDec>s, <statements>.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // First we finish populating the symbol table by adding local variables.
    while(data->current->value.nt_val == NT_VAR_DEC) {
        compile_var_dec(data);
    }

    // Now at last we have the information we need to start generating VM code!
    char code[MAX_LINE_LENGTH];
    sprintf(code, "function %s.%s %d\n", data->class_name, sub_name,
            data->subroutine_table->num_allocated[VK_VAR]);
    fputs(code, data->output);

    // If we're a method or a constructor, we'll need to initialise the "this" memory segment.
    // If we're a method, it will be the first argument passed in by the subroutine call and hence stored in argument 0.
    if (sub_kind == KW_METHOD) {
        sprintf(code, "push argument 0\n"
                                    "pop pointer 0\n");
        fputs(code, data->output);
    }
    // If we're a constructor, we will need to allocate space on the heap.
    else if (sub_kind == KW_CONSTRUCTOR) {
        sprintf(code, "push constant %d\n"
                      "call Memory.alloc 1\n"
                      "pop pointer 0\n", data->class_table->num_allocated[VK_FIELD]);
        fputs(code, data->output);
    }

    // Now we generate code for the actual subroutine body.
    compile_statements(data);

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Add variable(s) from given <varDec> to subroutine symbol table.
void compile_var_dec(CompileData *data) {
    // AST format: type keyword, list of identifiers.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Copy variable type.
    char *type = type_to_string(data->current);
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Add variables to symbol table.
    while(!(data->current->close_tag)) {
        add_to_table(data->subroutine_table, data->current->value.str_val, type, VK_VAR);
        advance_tag(&(data->current), &(data->lookahead), data->input);
    }

    // Cleanup, skip closing tag.
    free(type);
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <statements> with no side effects.
void compile_statements(CompileData *data) {
    // AST format: As in CST.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    while(!(data->current->close_tag)) {
        switch (data->current->value.nt_val) {
            case NT_LET_STATEMENT: compile_let_statement(data); break;
            case NT_DO_STATEMENT: compile_do_statement(data); break;
            case NT_IF_STATEMENT: compile_if_statement(data); break;
            case NT_WHILE_STATEMENT: compile_while_statement(data); break;
            case NT_RETURN_STATEMENT: compile_return_statement(data); break;
            default: printf("Unexpected tag in <statements>.\n"); exit(EXIT_FAILURE);
        }
    }

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <letStatement> with no side effects.
void compile_let_statement(CompileData *data) {
    // AST format: destination variable, [<expression>] if present, right-hand side <expression>.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
    bool brackets = false;
    char code[MAX_LINE_LENGTH];

    // Comment for debugging.
    fputs("// Compiling <letStatement>\n", data->output);

    // Look up destination variable in symbol tables and advance.
    TableEntry *entry;
    int table_pos = get_table_entry(data->subroutine_table, data->current->value.str_val);
    if (table_pos != -1) {
        entry = data->subroutine_table->table_array[table_pos];
    } else {
        table_pos = get_table_entry(data->class_table, data->current->value.str_val);
        entry = data->class_table->table_array[table_pos];
    }
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // If there are square brackets present, evaluate their contents and leave the address we need on top of the stack.
    if (data->current->type == SYMBOL) {
        brackets = true;
        advance_tag(&(data->current), &(data->lookahead), data->input);
        compile_expression(data);
        char buffer[MAX_LINE_LENGTH];
        var_to_string(entry, buffer);
        sprintf(code, "push %s\n"
                      "add\n", buffer);
        fputs(code, data->output);
        advance_tag(&(data->current), &(data->lookahead), data->input);
    }

    // Now evaluate the value to assign and leave it on the stack.
    compile_expression(data);

    // Now actually store the value.
    if (brackets) { // Here we have the values we need in the wrong order, so we use temp 0 as a holding facility.
        fputs("pop temp 0\n"
              "pop pointer 1\n"
              "push temp 0\n"
              "pop that 0\n", data->output);
    } else {
        char buffer[MAX_LINE_LENGTH];
        var_to_string(entry, buffer);
        sprintf(code, "pop %s\n", buffer);
        fputs(code, data->output);
    }

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <ifStatement> with no side effects.
void compile_if_statement(CompileData *data) {
    // AST format: Condition <expression>, <statements> in if clause, <statements> in else clause (if present)
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
    char code[MAX_LINE_LENGTH];

    // Define auto-incrementing labels.
    static int label_number = 0;
    char end_if_label[MAX_LINE_LENGTH];
    sprintf(end_if_label, "end_if_%d", label_number);
    char end_else_label[MAX_LINE_LENGTH];
    sprintf(end_else_label, "end_else_%d", label_number);
    label_number++;

    // Comment for debugging.
    fputs("// Compiling <ifStatement>\n", data->output);

    // Check the if condition and jump past the if statement if it's false.
    compile_expression(data);
    sprintf(code, "not\n"
                                "if-goto %s\n", end_if_label);
    fputs(code, data->output);

    // Output code for the <statements>.
    compile_statements(data);

    // If no else clause is present, output the label and we're done.
    if (data->current->close_tag) {
        sprintf(code, "label %s\n", end_if_label);
        fputs(code, data->output);
    } else { // Otherwise, output code for the else clause with appropriate label placement.
        sprintf(code, "goto %s\n"
                      "label %s\n", end_else_label, end_if_label);
        fputs(code, data->output);
        compile_statements(data);
        sprintf(code, "label %s\n", end_else_label);
        fputs(code, data->output);
    }

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <whileStatement> with no side effects.
void compile_while_statement(CompileData *data) {
    // AST format: Condition <expression>, <statements> in loop body.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
    char code[MAX_LINE_LENGTH];

    // Define auto-incrementing labels.
    static int label_number = 0;
    char start_loop_label[MAX_LINE_LENGTH];
    sprintf(start_loop_label, "start_while_%d", label_number);
    char end_loop_label[MAX_LINE_LENGTH];
    sprintf(end_loop_label, "end_while_%d", label_number);
    label_number++;

    // Output label for start of loop.
    sprintf(code, "label %s\n", start_loop_label);
    fputs(code, data->output);

    // Output code to check the loop condition, then break out of the loop if it's false.
    compile_expression(data);
    sprintf(code, "not\n"
                         "if-goto %s\n", end_loop_label);
    fputs(code, data->output);

    // Output code for the loop body.
    compile_statements(data);

    // Output code to loop and label to break out of loop.
    sprintf(code, "goto %s\n"
                  "label %s\n", start_loop_label, end_loop_label);
    fputs(code, data->output);

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <doStatement> with no side effects.
void compile_do_statement(CompileData *data) {
    // AST format: Just the subroutine call.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Comment for debugging.
    fputs("// Compiling <doStatement>\n", data->output);

    compile_sub_call(data);
    fputs("pop temp 0\n", data->output); // Discard return value.

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs all VM code for the current <returnStatement> with no side effects.
void compile_return_statement(CompileData *data) {
    // AST format: <expression> to return (if present).
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Comment for debugging.
    fputs("// Compiling <returnStatement>\n", data->output);

    // Leave return value on the stack (or 0 if there is no return value, just to avoid stack underflow).
    if (!(data->current->close_tag)) {
        compile_expression(data);
    } else {
        fputs("push constant 0\n", data->output);
    }

    // Actually return.
    fputs("return\n", data->output);

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs VM code that evaluates the current <expression> and leaves its value on top of the stack.
void compile_expression(CompileData *data) {
    // AST format: As in original, <term>, {<op>, <term>}.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    compile_term(data);
    while(!(data->current->close_tag)) {
        char current_op = data->current->value.str_val[0];
        advance_tag(&(data->current), &(data->lookahead), data->input);
        compile_term(data);

        // We now have the results of both terms on top of the stack, so we just need to translate the operation.
        switch(current_op) {
            case '+': fputs("add\n", data->output); break;
            case '-': fputs("sub\n", data->output); break;
            case '*': fputs("call Math.multiply 2\n", data->output); break;
            case '/': fputs("call Math.divide 2\n", data->output); break;
            case '&': fputs("and\n", data->output); break;
            case '|': fputs("or\n", data->output); break;
            case '<': fputs("lt\n", data->output); break;
            case '>': fputs("gt\n", data->output); break;
            case '=': fputs("eq\n", data->output); break;
            default: printf("Unsupported <op> in <expression>.\n"); exit(EXIT_FAILURE);
        }
    }

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs VM code that calls the given subroutine and leaves the result on the stack.
void compile_sub_call(CompileData *data) {
    // AST format: identifier, ['.', identifier], <expressionList>
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
    char code[MAX_LINE_LENGTH];
    char sub_name[MAX_LINE_LENGTH];
    char first_identifier[MAX_LINE_LENGTH];
    bool is_method = false;

    // Take a temporary copy of the current identifier (the one before the dot) and advance.
    strcpy(first_identifier, data->current->value.str_val);
    advance_tag(&(data->current), &(data->lookahead), data->input);

    // Subroutine calls are in the form [class name].[subroutine name], [variable name].[subroutine name] (for methods),
    // or [subroutine name] (for method calls to the current class). The actual name of the subroutine is in the form
    // [class name].[subroutine name]. We need to a) find the name and store it in sub_name, and b) if this is a method
    // call, push the owning class instance onto the stack as the first argument (where it will become the value of
    // "this").
    if (data->current->type != SYMBOL) {
        is_method = true;
        fputs("push pointer 0\n", data->output);
        sprintf(sub_name, "%s.%s", data->class_name, first_identifier);
    }
    else {
        // Advance past the dot.
        advance_tag(&(data->current), &(data->lookahead), data->input);

        // Use the symbol tables to check if the thing before the dot is a known variable name. If it is one,
        // set entry to its entry.
        TableEntry *entry;
        int table_pos = get_table_entry(data->subroutine_table, first_identifier);
        if (table_pos != -1) {
            is_method = true;
            entry = data->subroutine_table->table_array[table_pos];
        } else {
            table_pos = get_table_entry(data->class_table, first_identifier);
            if (table_pos != -1) {
                is_method = true;
                entry = data->class_table->table_array[table_pos];
            }
        }

        // In this case we are calling a method, and we have the "owning" variable's table entry. We must push the
        // variable's value (i.e. its address in RAM) onto the stack.
        if (is_method) {
            char buffer[MAX_LINE_LENGTH];
            var_to_string(entry, buffer);
            sprintf(code, "push %s\n", buffer);
            fputs(code, data->output);
            sprintf(sub_name, "%s.%s", entry->type, data->current->value.str_val);
        }
        // Otherwise, we are calling a constructor or a function. We need no special preparation and its name is
        // exactly what it appears to be.
        else {
            sprintf(sub_name, "%s.%s", first_identifier, data->current->value.str_val);
        }

        // Either way, advance past the second identifier.
        advance_tag(&(data->current), &(data->lookahead), data->input);
    }

    // Output code that leaves all the non-this arguments on the stack.
    int number_args = compile_expression_list(data);
    if (is_method) {
        number_args++;
    }

    // Output the actual function call.
    sprintf(code, "call %s %d\n", sub_name, number_args);
    fputs(code, data->output);

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Outputs VM code that evaluates each expression in the given <expressionList>, leaving the results on the stack, and
// returns the total number of expressions.
int compile_expression_list(CompileData *data) {
    // Children in AST: Just the <expression>s.
    // AST format: As in original, <term>, {<op>, <term>}.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
    int expression_count = 0;

    while (!(data->current->close_tag)) {
        compile_expression(data);
        expression_count++;
    }

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
    return expression_count;
}

// Outputs VM code that evaluates the current <term> and leaves its value on top of the stack.
void compile_term(CompileData *data) {
    // AST syntax: integerLiteral | stringLiteral | 'true' | 'false' | 'null' | 'this' |
    //             identifier, ['[', <expression>, ']'] |
    //             <expression> |
    //             ('-' | '~'), <term> |
    //             <subroutineCall>
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
    char code[MAX_LINE_LENGTH];

    if (data->current->type == INTEGER_LITERAL) {
        sprintf(code, "push constant %d\n", data->current->value.int_val);
        fputs(code, data->output);
        advance_tag(&(data->current), &(data->lookahead), data->input);
    } else if (data->current->type == STRING_LITERAL) {
        char *literal = data->current->value.str_val;

        // dare you enter my magical realm
        sprintf(code, "push constant %d\n"
                      "call String.new 1\n", strlen(literal));
        fputs(code, data->output);
        for(int i = 0; i < strlen(literal); i++) {
            sprintf(code, "push constant %d\n"
                          "call String.appendChar 2\n", literal[i]);
            fputs(code, data->output);
        }

        // We're done! Yay! The value of a String set to the given literal is now on the stack, with no long-term
        // repercussions whatsoever! *turns and walks away from burning building*
        advance_tag(&(data->current), &(data->lookahead), data->input);
    } else if (data->current->type == KEYWORD) {
        switch(data->current->value.key_val) {
            case KW_NULL:
            case KW_FALSE: fputs("push constant 0\n", data->output); break;
            case KW_TRUE: fputs("push constant 1\n"
                                "neg\n", data->output); break;
            case KW_THIS: fputs("push pointer 0\n", data->output); break;
            default: printf("Unsupported keyword in <term>.\n"); exit(EXIT_FAILURE);
        }
        advance_tag(&(data->current), &(data->lookahead), data->input);
    } else if (data->current->type == IDENTIFIER) {
        // In this case it's a variable. Pull the symbol table entry and advance the tag.
        TableEntry *entry;
        int table_pos = get_table_entry(data->subroutine_table, data->current->value.str_val);
        if (table_pos == -1) {
            table_pos = get_table_entry(data->class_table, data->current->value.str_val);
            entry = data->class_table->table_array[table_pos];
        } else {
            entry = data->subroutine_table->table_array[table_pos];
        }
        advance_tag(&(data->current), &(data->lookahead), data->input);

        // In all cases, we'll need to push the variable's value onto the stack.
        char buffer[MAX_LINE_LENGTH];
        var_to_string(entry, buffer);
        sprintf(code, "push %s\n", buffer);
        fputs(code, data->output);

        // If we have [<expression>] after the identifier, then we need to read x[i] as *(x+i).
        if (!(data->current->close_tag)) {
            // Skip the brackets and put the result of the <expression> onto the stack.
            advance_tag(&(data->current), &(data->lookahead), data->input);
            compile_expression(data);
            advance_tag(&(data->current), &(data->lookahead), data->input);
            fputs("add\n"
                  "pop pointer 1\n"
                  "push that 0\n", data->output);
        }
    } else if (data->current->type == NON_TERMINAL && data->current->value.nt_val == NT_EXPRESSION) {
        compile_expression(data);
    } else if (data->current->type == SYMBOL) {
        char op_value = data->current->value.str_val[0];
        advance_tag(&(data->current), &(data->lookahead), data->input);
        compile_term(data);
        if (op_value == '-') {
            fputs("neg\n", data->output);
        } else { // Must be '~'
            fputs("not\n", data->output);
        }
    } else { // In this case it must be a subroutine call.
        compile_sub_call(data);
    }

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Add all given arguments to subroutine symbol table.
void compile_parameter_list(CompileData *data) {
    // AST syntax: {<type>, identifier}.
    // Skip opening tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);

    while (!(data->current->close_tag)) {
        char *type = type_to_string(data->current);
        add_to_table(data->subroutine_table, data->lookahead->value.str_val,
                     type, VK_ARGUMENT);
        free(type);
        advance_tag(&(data->current), &(data->lookahead), data->input);
        advance_tag(&(data->current), &(data->lookahead), data->input);
    }

    // Skip closing tag.
    advance_tag(&(data->current), &(data->lookahead), data->input);
}

// Converts the current tag (assumed to be a keyword or identifier) into a string and returns the string. NB the string
// will need to be freed in order to avoid a memory leak.
char *type_to_string(Tag *current) {
    char *type;
    if (current->type == IDENTIFIER) {
        type = malloc(strlen(current->value.str_val) + 1);
        strcpy(type, current->value.str_val);
    } else {
        type = malloc(8);
        switch (current->value.key_val) {
            case KW_INT: strcpy(type, "int"); break;
            case KW_BOOLEAN: strcpy(type, "boolean"); break;
            case KW_CHAR: strcpy(type, "char"); break;
            default: printf("Bad variable type in <varDec>.\n"); exit(EXIT_FAILURE);
        }
    }
    return type;
}

void var_to_string(TableEntry *var_entry, char *buffer) {
    switch(var_entry->kind) {
        case VK_FIELD: sprintf(buffer, "this %d", var_entry->offset); break;
        case VK_VAR: sprintf(buffer, "local %d", var_entry->offset); break;
        case VK_STATIC: sprintf(buffer, "static %d", var_entry->offset); break;
        case VK_ARGUMENT: sprintf(buffer, "argument %d", var_entry->offset); break;
        default: printf("Impossible enum value."); exit(EXIT_FAILURE);
    }
}