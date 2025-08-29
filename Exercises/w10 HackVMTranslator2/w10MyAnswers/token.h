#include <stdio.h>
#include <stdbool.h>

enum HackTokenType {
    KEYWORD,
    INTEGER_LITERAL,
    IDENTIFIER,
    NEWLINE
}; typedef enum HackTokenType HackTokenType;

enum Keyword {
    PUSH, POP, ADD, SUB, NEG, AND, OR, NOT, EQ, GT, LT,
    LOCAL, CONSTANT, KW_THIS, THAT, POINTER, ARGUMENT, STATIC, TEMP,
    LABEL, GOTO, IFGOTO,
    FUNCTION, CALL, RETURN
}; typedef enum Keyword Keyword;

/* A union is a special type that uses one spot in memory to store one of several
 * different variables of different types. Here, TokenData can store either a Keyword,
 * an int, a character, or a string. Assigning to e.g. key_val will overwrite what's
 * stored in int_val, and there's no built-in way to tell whether what's currently stored
 * there is e.g. an int or a string --- we'll keep track of that with the HackTokenType enum.
 *
 * Usage examples: Suppose data is a TokenData variable.
 *      data.int_val = 42; printf("%d", data.int_val);    // Prints 42.
 *      data.str_val = "foo"; printf("%s", data.str_val); // Prints "foo".
 */
union TokenData {
    Keyword key_val;
    int int_val;
    char *str_val;
}; typedef union TokenData TokenData;

struct Token {
    HackTokenType type;
    TokenData value;
}; typedef struct Token Token;

// Returns a pointer to a newly-allocated Token object.
Token *malloc_token();
// Frees a Token pointer and everything associated with it.
void free_token(Token *);
// Writes a token to [output] in the form "type,value".
void write_token(Token *, FILE *);
// Returns false if there are no more tokens in input, otherwise stores the next token in dest and returns true.
bool read_token(Token *, FILE *);

// Maximum token length
#define MAX_LINE_LENGTH 256