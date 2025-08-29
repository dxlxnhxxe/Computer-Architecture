#include <stdio.h>
#include <stdbool.h>

enum JackTagType {
    KEYWORD,
    SYMBOL,
    INTEGER_LITERAL,
    STRING_LITERAL,
    IDENTIFIER,
    NON_TERMINAL
}; typedef enum JackTagType JackTagType;
#define NUM_JTTS 6
extern const char *TagTypeToString[];

enum Keyword {
    KW_CLASS, KW_CONSTRUCTOR, KW_FUNCTION, KW_METHOD, KW_FIELD, KW_STATIC,
    KW_VAR, KW_INT, KW_CHAR, KW_BOOLEAN, KW_VOID,
    KW_TRUE, KW_FALSE, KW_NULL, KW_THIS,
    KW_LET, KW_DO, KW_IF, KW_ELSE, KW_WHILE, KW_RETURN
}; typedef enum Keyword Keyword;
#define NUM_KWS 21
extern const char *KeywordToString[];

enum NonTerminal {
    NT_CLASS, NT_CLASS_VAR_DEC, NT_SUBROUTINE_DEC, NT_PARAMETER_LIST, NT_SUBROUTINE_BODY, NT_VAR_DEC,
    NT_STATEMENTS, NT_LET_STATEMENT, NT_IF_STATEMENT, NT_WHILE_STATEMENT, NT_DO_STATEMENT, NT_RETURN_STATEMENT,
    NT_EXPRESSION, NT_TERM, NT_EXPRESSION_LIST, NT_OP, NT_UNARY_OP, NT_TOKENS, NT_SUBROUTINE_CALL
}; typedef enum NonTerminal NonTerminal;
#define NUM_NTS 19
extern const char *NonTerminalToString[];

/* A union is a special type that uses one spot in memory to store one of several
 * different variables of different types. Here, TagData can store either a Keyword,
 * an int, a character, or a string. Assigning to e.g. key_val will overwrite what's
 * stored in int_val, and there's no built-in way to tell whether what's currently stored
 * there is e.g. an int or a string --- we'll keep track of that with the JackTagType enum.
 *
 * Usage examples: Suppose data is a TagData variable.
 *      data.int_val = 42; printf("%d", data.int_val);    // Prints 42.
 *      data.str_val = "foo"; printf("%s", data.str_val); // Prints "foo".
 */
union TagData {
    Keyword key_val;
    NonTerminal nt_val;
    int int_val;
    char *str_val;
}; typedef union TagData TagData;

struct Tag {
    JackTagType type;
    TagData value;
    bool close_tag;
}; typedef struct Tag Tag;

// Returns a pointer to a newly-allocated Tag object.
Tag *malloc_tag();
// Frees a Tag pointer and everything associated with it.
void free_tag(Tag **var);
// Writes a token to [output] in the form "type,value".
void write_tag(Tag *var, FILE *output);
// Returns false if there are no more tokens in input, otherwise stores the next token in var and returns true.
bool read_tag(Tag **var, FILE *input);
// Copies the current tag into the provided output file. Updates the lookahead tag to be the next unread tag in the
// input, and updates the current tag to be the old lookahead tag. Sets lookahead to NULL if there is no next tag.
void copy_tag(Tag **current, Tag **lookahead, FILE *input, FILE *output);
// Behaves like copy_tag, but does not copy the current tag into the output file.
void advance_tag(Tag **current, Tag **lookahead, FILE *input);
// Writes a new non-terminal tag with the given value into the given output file, making it an end tag if the
// corresponding boolean argument is true.
void write_non_terminal(NonTerminal nt, bool end, FILE *output);

// Maximum token length
#define MAX_LINE_LENGTH 256