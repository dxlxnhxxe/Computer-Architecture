#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "token.h"

// C commands for folder handling are different between Windows and Linux.
// *In theory* the Linux version will also work for Macs, but I can't promise anything
// due to a lack of suitable test environments.
#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <fnmatch.h>
#define MAX_PATH 2500
#endif

void compile_folder(char *input_path, FILE *output);
void compile_file(char *input_path, FILE *output, bool standalone);

// System functions
bool is_folder(const char *path);
int list_vm_files(const char *path, char ***list);

// Lexing functions
void lex_file(FILE *input, FILE *output);
void lex_line(const char *line, FILE *output);
int lex_token(Token *dest, const char *line);

// Parsing functions
void parse_file(char *filename, FILE *input, FILE *output, bool standalone);
int get_next_instruction(Token *dest[], FILE *input);
void parse_instruction(Token *instruction[], char *filename, FILE *output);
void parse_push(char *filename, Token *segment, Token *address, char *dest);
void parse_pop(char *filename, Token *segment, Token *address, char *dest);
void parse_add(char *dest);
void parse_sub(char *dest);
void parse_neg(char *dest);
void parse_and(char *dest);
void parse_or(char *dest);
void parse_not(char *dest);
void parse_eq(char *filename, char *dest);
void parse_lt(char *filename, char *dest);
void parse_gt(char *filename, char *dest);
void parse_label(char *filename, Token *label, char *dest);
void parse_goto(char *filename, Token *label, char *dest);
void parse_ifgoto(char *filename, Token *label, char *dest);
void parse_call(char *filename, Token *name, Token *args, char *dest);
void parse_function(char *filename, Token *name, Token *local_vars, char *dest);
void parse_return(char *dest);
void parse_load_data(char *filename, Token *segment, Token *address, char *dest);
void get_next_label_name(char *filename, char *dest);
void get_function_label(char *function_name, char *dest);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Please supply two arguments: an input .vm file or folder, and an output .asm file.\n");
        exit(EXIT_FAILURE);
    }

    char *input_name = argv[1];
    char *output_name = argv[2];

    FILE *output = fopen(output_name, "w");
    if (output == NULL) {
        exit(EXIT_FAILURE);
    }

    if (is_folder(input_name)) {
        compile_folder(input_name, output);
    } else {
        compile_file(input_name, output, true);
    }

    fclose(output);

    return EXIT_SUCCESS;
}

void compile_file(char *input_path, FILE *output, bool standalone) {
    FILE *lex_output = fopen("temp.lex", "w");
    if (lex_output == NULL) {
        printf("Error opening .lex output file.");
        exit(EXIT_FAILURE);
    }

    FILE *input = fopen(input_path, "r");
    if (input == NULL) {
        printf("Error opening input file %s in single-file mode.", input_path);
        exit(EXIT_FAILURE);
    }

    lex_file(input, lex_output);

    fclose(input);
    fclose(lex_output);

    FILE *lex_input = fopen("temp.lex", "r");
    if (lex_input == NULL) {
        printf("Error reopening .lex file for input.");
        exit(EXIT_FAILURE);
    }

    parse_file(input_path, lex_input, output, standalone);
    fclose(lex_input);
}

void compile_folder(char *input_path, FILE *output) {
    char **list;
    int no_files = list_vm_files(input_path, &list);

    // Send assembly code to initialise SP to 256 and call Sys.init to output
    char code[400];
    char init_label[200];
    get_function_label("Sys.init", init_label);
    sprintf(code, "@261\n"
                  "D=A\n"
                  "@LCL\n" // We set LCL rather than the stack pointer since the
                  "M=D\n"  // function code will initialise SP to LCL.
                  "@%s\n"
                  "0;JMP", init_label);
    fputs(code, output);

    for (int i=0; i<no_files; i++) {
        FILE *lex_output = fopen("temp.lex", "w");
        if (lex_output == NULL) {
            printf("Error opening .lex output file.");
            exit(EXIT_FAILURE);
        }

        FILE *input = fopen(list[i], "r");
        if (input == NULL) {
            printf("Error opening input file %s for lexing.",list[i]);
            exit(EXIT_FAILURE);
        }

        lex_file(input, lex_output);

        fclose(input);
        fclose(lex_output);

        FILE *lex_input = fopen("temp.lex", "r");
        if (lex_input == NULL) {
            printf("Error reopening .lex file for input.");
            exit(EXIT_FAILURE);
        }

        char filename[MAX_PATH] = "";
        int slash_pos = 0;
        while(list[i][slash_pos] != '\\' && list[i][slash_pos] != '/') {
            slash_pos++;
        }
        slash_pos++;
        int pos = 0;
        while(list[i][pos+slash_pos] != '\0') {
            filename[pos] = list[i][pos+slash_pos];
            pos++;
        }
        parse_file(filename, lex_input, output, false);

        fclose(lex_input);
        free(list[i]);
    }
    free(list);
}

// Note that folder names can have .s in them, so we do need to do this the clever way.
bool is_folder(const char *path) {
#ifdef _WIN32
    // Supplied by windows.h
    DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        printf("Input file/folder not found.");
        exit(EXIT_FAILURE);
    }
    return attributes & FILE_ATTRIBUTE_DIRECTORY;
#else
    // Supplied by sys/stat.h
    struct stat buffer;
    if (stat(path, &buffer) != 0) {
        printf("Input file/folder not found.");
        exit(EXIT_FAILURE);
    };
    return S_ISDIR(buffer.st_mode);
#endif
}

// Sets **list to a list of paths of all .vm files in the folder path. The return value is the length of the list
// (counting from 0). Full disclosure, this is beyond my C abilities and I turned to every corner of the Internet for
// help. I would feel bad about this if I taught C, but I don't, so I don't! :-D
int list_vm_files(const char *path, char ***list) {
#ifdef _WIN32
    // This is all from windows.h.
    WIN32_FIND_DATA fd_file;
    HANDLE hFind = NULL;

    char s_path[2048];
    sprintf(s_path, "%s\\*.vm", path);
    int length = 0;

    hFind = FindFirstFile(s_path, &fd_file);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        printf("No .vm files found in specified folder.");
        exit(EXIT_FAILURE);
    }
    do {
        length++;
    } while(FindNextFile(hFind, &fd_file));
    FindClose(hFind);

    *list = (char **)malloc(length * sizeof(char *));
    hFind = FindFirstFile(s_path, &fd_file);
    for(int i=0; i<length; i++) {
        (*list)[i] = (char *)malloc((MAX_PATH+1)*sizeof(char));
        sprintf((*list)[i], "%s\\%s", path, fd_file.cFileName);
        FindNextFile(hFind, &fd_file);
    }
    FindClose(hFind);

    return length;
#else
    // This is all from dirent.h and fnmatch.h
    DIR *dir;
    struct dirent *entry;

    dir = opendir(path);
    if (dir == NULL) {
        printf("Error opening folder");
        exit(EXIT_FAILURE);
    }

    int length = 0;
    entry = readdir(dir);
    while (entry != NULL) {
        if (fnmatch("*.vm", entry->d_name, FNM_CASEFOLD) == 0) {
            length++;
        }
        entry = readdir(dir);
    }
    closedir(dir);

    *list = (char **)malloc(length * sizeof(char *));
    int i = 0;
    dir = opendir(path);
    entry = readdir(dir);
    while (entry != NULL) {
        if (fnmatch("*.vm", entry->d_name, FNM_CASEFOLD) == 0) {
            (*list)[i] = (char *)malloc((MAX_PATH+1)*sizeof(char));
            sprintf((*list)[i], "%s/%s", path, entry->d_name);
            i++;
        }
        entry = readdir(dir);
    }
    closedir(dir);

    return length;
#endif
}

// Outputs a tokenised version of input to output.
void lex_file(FILE *input, FILE *output) {
    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, input) != NULL) {
        lex_line(line, output);
    }
}

// Reads the next line from input, tokenises it, and writes the resulting tokens to output.
void lex_line(const char *line, FILE *output) {
    int pos = 0;
    bool tokens_on_line = false;

    // In each iteration of this loop, everything up to line[pos] has been lexed.
    while ((line[pos] != '\n') && (line[pos] != '\r')) {
        // Ignore all whitespace.
        if (line[pos] == ' ') {
            pos++;
            continue;
        }

        // Ignore all comments.
        if (line[pos] == '/') {
            break;
        }

        // Otherwise, we have at least one non-newline token, so lex it.
        tokens_on_line = true;
        Token *next = malloc_token();
        pos += lex_token(next, line + pos);
        write_token(next, output);
        free_token(next);
    }

    // Ignore empty lines, but otherwise lex the newline at the end.
    if (tokens_on_line) {
        Token *next = malloc_token();
        lex_token(next, "\n");
        write_token(next, output);
        free_token(next);
    }
}

// Reads the next token from a non-empty, non-label, non-comment line into dest, then returns the number of characters
// in that token.
int lex_token(Token *dest, const char *line) {
    int length;

    if (line[0] == '\n') {
        dest->type = NEWLINE;
        length = 1;
    } else if (line[0] >= '0' && line[0] <= '9') {
        // Identifiers can't start with integers, so this must be an integer literal
        dest->type = INTEGER_LITERAL;
        char literal[MAX_LINE_LENGTH];
        for(length = 0; line[length] >= '0' && line[length] <= '9'; length++) {
            literal[length] = line[length];
        }
        literal[length] = '\0';
        dest->value.int_val = strtol(literal, NULL, 10);
    } else { // We either have an identifier or a keyword
        // Either way, it keeps going until reaching either a space, a newline. (Per the definition of an identifier
        // token, if there's a // before a space, it counts as part of the identifier rather than a comment.)
        length = 0;
        while(line[length] != ' ' && line[length] != '\n' && line[length] != '\r'){
            length++;
        }

        dest->type = KEYWORD; // Default
        if (strncmp(line, "push", length) == 0) {
            dest->value.key_val = PUSH;
        } else if (strncmp(line, "pop", length) == 0) {
            dest->value.key_val = POP;
        } else if (strncmp(line, "add", length) == 0) {
            dest->value.key_val = ADD;
        } else if (strncmp(line, "sub", length) == 0) {
            dest->value.key_val = SUB;
        } else if (strncmp(line, "neg", length) == 0) {
            dest->value.key_val = NEG;
        } else if (strncmp(line, "and", length) == 0) {
            dest->value.key_val = AND;
        } else if (strncmp(line, "or", length) == 0) {
            dest->value.key_val = OR;
        } else if (strncmp(line, "not", length) == 0) {
            dest->value.key_val = NOT;
        } else if (strncmp(line, "eq", length) == 0) {
            dest->value.key_val = EQ;
        } else if (strncmp(line, "gt", length) == 0) {
            dest->value.key_val = GT;
        } else if (strncmp(line, "lt", length) == 0) {
            dest->value.key_val = LT;
        } else if (strncmp(line, "local", length) == 0) {
            dest->value.key_val = LOCAL;
        } else if (strncmp(line, "constant", length) == 0) {
            dest->value.key_val = CONSTANT;
        } else if (strncmp(line, "this", length) == 0) {
            dest->value.key_val = KW_THIS;
        } else if (strncmp(line, "that", length) == 0) {
            dest->value.key_val = THAT;
        } else if (strncmp(line, "pointer", length) == 0) {
            dest->value.key_val = POINTER;
        } else if (strncmp(line, "argument", length) == 0) {
            dest->value.key_val = ARGUMENT;
        } else if (strncmp(line, "static", length) == 0) {
            dest->value.key_val = STATIC;
        } else if (strncmp(line, "temp", length) == 0) {
            dest->value.key_val = TEMP;
        } else if (strncmp(line, "label", length) == 0) {
            dest->value.key_val = LABEL;
        } else if (strncmp(line, "goto", length) == 0) {
            dest->value.key_val = GOTO;
        } else if (strncmp(line, "if-goto", length) == 0) {
            dest->value.key_val = IFGOTO;
        } else if (strncmp(line, "function", length) == 0) {
            dest->value.key_val = FUNCTION;
        } else if (strncmp(line, "call", length) == 0) {
            dest->value.key_val = CALL;
        } else if (strncmp(line, "return", length) == 0) {
            dest->value.key_val = RETURN;
        } else { // We've now ruled out all the keywords, so it must be an identifier.
            dest->type = IDENTIFIER;
            dest->value.str_val = (char *)malloc(length+1);
            strncpy(dest->value.str_val, line, length);
            dest->value.str_val[length] = '\0'; // Strncpy doesn't null-terminate.
        }
    }
    return length;
}

// Input should be a tokenised file. Writes Hack assembly code to output.
void parse_file(char *filename, FILE *input, FILE *output, bool standalone) {
    Token *instruction[MAX_LINE_LENGTH];
    if (standalone) {
        // Send code to output to initialise SP. Comment out to make week 10 test scripts work.
/*        fputs("@256\n"
                "D=A\n"
                "@SP\n"
                "M=D\n", output); */
    }
    while (1) {
        // Get the next instruction.
        int length = get_next_instruction(instruction, input);
        // If we just have an EOF, free the token and finish.
        if (length == 0) {
            free_token(instruction[0]);
            break;
        }
        // Otherwise, actually parse the instruction, free the token and repeat.
        parse_instruction(instruction, filename, output);
        for(int i=0; i<length; i++) {
            free_token(instruction[i]);
        }
    }
    if (standalone) {
        // Send code to output to end with infinite loop.
        fputs("(HaltInfiniteLoop)\n"
              "@HaltInfiniteLoop\n"
              "0;JMP", output);
    }
}

// Copies a list of tokens corresponding to the next Hack VM instruction into dest, omitting the newline. Returns number
// of tokens.
int get_next_instruction(Token *dest[], FILE *input) {
    dest[0] = malloc_token();
    bool instruction_exists = read_token(dest[0], input);
    if (!instruction_exists) {
        return 0;
    }
    int length = 1;
    while (1) {
        dest[length] = malloc_token();
        read_token(dest[length], input);
        if (dest[length]->type == NEWLINE) {
            free_token(dest[length]);
            break;
        }
        length++;
    }
    return length;
}

// Parse the given VM instruction and write the corresponding assembly code to the output file.
void parse_instruction(Token *instruction[], char *filename, FILE *output) {
    char code_to_output[1000000] = "";

    // We can tell the entire syntax of the instruction from the first token, which should be a keyword.
    if (instruction[0]->type != KEYWORD) {
        printf("Malformed instruction!");
        exit(EXIT_FAILURE);
    } switch (instruction[0]->value.key_val) {
        case PUSH:     parse_push(filename, instruction[1], instruction[2], code_to_output); break;
        case POP:      parse_pop(filename, instruction[1], instruction[2], code_to_output); break;
        case ADD:      parse_add(code_to_output); break;
        case SUB:      parse_sub(code_to_output); break;
        case NEG:      parse_neg(code_to_output); break;
        case AND:      parse_and(code_to_output); break;
        case OR:       parse_or(code_to_output); break;
        case NOT:      parse_not(code_to_output); break;
        case EQ:       parse_eq(filename, code_to_output); break;
        case LT:       parse_lt(filename, code_to_output); break;
        case GT:       parse_gt(filename, code_to_output); break;
        case LABEL:    parse_label(filename, instruction[1], code_to_output); break;
        case GOTO:     parse_goto(filename, instruction[1], code_to_output); break;
        case IFGOTO:   parse_ifgoto(filename, instruction[1], code_to_output); break;
        case FUNCTION: parse_function(filename, instruction[1], instruction[2], code_to_output); break;
        case CALL:     parse_call(filename, instruction[1], instruction[2], code_to_output); break;
        case RETURN:   parse_return(code_to_output); break;
        default: printf("Malformed instruction!"); exit(EXIT_FAILURE);
    }

    fputs(code_to_output, output);
    fflush(output);
}

// Append assembly code for an "add" instruction into dest.
void parse_add(char *dest) {
    strcat(dest, "// add\n"
                 "@SP\n"
                 "M=M-1\n"
                 "A=M\n"
                 "D=M\n"
                 "@SP\n"
                 "A=M-1\n"
                 "M=M+D\n");
}

// Append assembly code for the instruction "push [segment] [address]" into dest.
void parse_push(char *filename, Token *segment, Token *address, char *dest) {
    if (segment->type != KEYWORD) {
        printf("Malformed instruction!");
        exit(EXIT_FAILURE);
    }
    if (segment->value.key_val == CONSTANT) {
        char code[200] = "";
        sprintf(code, "//push\n"
                      "@%d\n"
                      "D=A\n"
                      "@SP\n"
                      "M=M+1\n"
                      "A=M-1\n"
                      "M=D\n", address->value.int_val);
        strcat(dest, code);
    } else {
        parse_load_data(filename, segment, address, dest);
        strcat(dest, "D=M\n"
                     "@SP\n"
                     "M=M+1\n"
                     "A=M-1\n"
                     "M=D\n");
    }
}

// Append assembly code for the instruction "pop [segment] [address]" into dest.
void parse_pop(char *filename, Token *segment, Token *address, char *dest) {
    if (segment->type != KEYWORD) {
        printf("Malformed instruction!");
        exit(EXIT_FAILURE);
    }
    strcat(dest, "//pop\n");
    parse_load_data(filename, segment, address, dest);
    strcat(dest, "D=A\n"
                 "@R13\n"
                 "M=D\n" // Now R13 contains the address we want to pop into
                 "@SP\n"
                 "M=M-1\n"
                 "A=M\n"
                 "D=M\n" // Now we have decremented SP and stored the value we want to pop in D
                 "@R13\n"
                 "A=M\n"
                 "M=D\n");
}

// Append assembly code for the instruction "sub" into dest.
void parse_sub(char *dest) {
    strcat(dest, "// sub\n"
                 "@SP\n"
                 "M=M-1\n"
                 "A=M\n"
                 "D=M\n"
                 "@SP\n"
                 "A=M-1\n"
                 "M=M-D\n");
}

// Append assembly code for the instruction "neg" into dest.
void parse_neg(char *dest) {
    strcat(dest, "// neg\n"
                 "@SP\n"
                 "A=M-1\n"
                 "D=-M\n"
                 "M=D\n");
}

// Append assembly code for the instruction "and" into dest.
void parse_and(char *dest) {
    strcat(dest, "// and\n"
                 "@SP\n"
                 "M=M-1\n"
                 "A=M\n"
                 "D=M\n"
                 "@SP\n"
                 "A=M-1\n"
                 "M=M&D\n");
}

// Append assembly code for the instruction "or" into dest.
void parse_or(char *dest) {
    strcat(dest, "// or\n"
                 "@SP\n"
                 "M=M-1\n"
                 "A=M\n"
                 "D=M\n"
                 "@SP\n"
                 "A=M-1\n"
                 "M=M|D\n");
}

// Append assembly code for the instruction "not" into dest.
void parse_not(char *dest) {
    strcat(dest, "// not\n"
                 "@SP\n"
                 "A=M-1\n"
                 "D=!M\n"
                 "M=D\n");
}

// Append assembly code for the instruction "eq" into dest.
void parse_eq(char *filename, char *dest) {
    char new_label1[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label1);
    char new_label2[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label2);

    char code[4*MAX_LINE_LENGTH+200];
    sprintf(code, "// eq\n"
                  "@SP\n"
                  "M=M-1\n"
                  "A=M\n"
                  "D=M\n"
                  "@SP\n"
                  "A=M-1\n"
                  "D=D-M\n" // D now contains RAM[SP-1] - RAM[SP-2], and SP has been decremented
                  "@%s\n"
                  "D;JEQ\n"
                  "@SP\n" // If we are here then RAM[SP-2] != RAM[SP-1], so write 0x0000 to RAM[SP-2]
                  "A=M-1\n"
                  "M=0\n"
                  "@%s\n"
                  "0;JMP\n"
                  "(%s)\n"
                  "@SP\n"
                  "A=M-1\n"
                  "M=-1\n" // If we are here then RAM[SP-2] == RAM[SP-1], so write 0xFFFF to RAM[SP-2]
                  "(%s)\n", new_label1, new_label2, new_label1, new_label2);
    strcat(dest, code);
}

// Append assembly code for the instruction "lt" into dest.
void parse_lt(char *filename, char *dest) {
    char new_label1[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label1);
    char new_label2[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label2);

    char code[4*MAX_LINE_LENGTH+200];
    sprintf(code, "// lt\n"
                  "@SP\n"
                  "M=M-1\n"
                  "A=M\n"
                  "D=M\n"
                  "@SP\n"
                  "A=M-1\n"
                  "D=D-M\n" // D now contains RAM[SP-1] - RAM[SP-2], and SP has been decremented
                  "@%s\n"
                  "D;JGT\n"
                  "@SP\n" // If we are here then RAM[SP-2] >= RAM[SP-1], so write 0x0000 to RAM[SP-2]
                  "A=M-1\n"
                  "M=0\n"
                  "@%s\n"
                  "0;JMP\n"
                  "(%s)\n"
                  "@SP\n"
                  "A=M-1\n"
                  "M=-1\n" // If we are here then RAM[SP-2] < RAM[SP-1], so write 0xFFFF to RAM[SP-2]
                  "(%s)\n", new_label1, new_label2, new_label1, new_label2);
    strcat(dest, code);
}

// Append assembly code for the instruction "gt" into dest.
void parse_gt(char *filename, char *dest) {
    char new_label1[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label1);
    char new_label2[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, new_label2);

    char code[4*MAX_LINE_LENGTH+200];
    sprintf(code, "// gt\n"
                  "@SP\n"
                  "M=M-1\n"
                  "A=M\n"
                  "D=M\n"
                  "@SP\n"
                  "A=M-1\n"
                  "D=D-M\n" // D now contains RAM[SP-1] - RAM[SP-2], and SP has been decremented
                  "@%s\n"
                  "D;JLT\n"
                  "@SP\n" // If we are here then RAM[SP-2] <= RAM[SP-1], so write 0x0000 to RAM[SP-2]
                  "A=M-1\n"
                  "M=0\n"
                  "@%s\n"
                  "0;JMP\n"
                  "(%s)\n"
                  "@SP\n"
                  "A=M-1\n"
                  "M=-1\n" // If we are here then RAM[SP-2] > RAM[SP-1], so write 0xFFFF to RAM[SP-2]
                  "(%s)\n", new_label1, new_label2, new_label1, new_label2);
    strcat(dest, code);
}

// Append assembly code for the instruction "label [label]" into dest.
void parse_label(char *filename, Token *label, char *dest) {
    char code[MAX_LINE_LENGTH+200] = "";
    sprintf(code, "// Label\n(manual$%s$%s)\n", filename, label->value.str_val);
    strcat(dest, code);
}

// Append assembly code for the instruction "goto [label]" into dest.
void parse_goto(char *filename, Token *label, char *dest) {
    char code[MAX_LINE_LENGTH+200] = "";
    sprintf(code, "// Goto\n@manual$%s$%s\n"
                  "0;JMP\n", filename, label->value.str_val);
    strcat(dest, code);
}

// Append assembly code for the instruction "if-goto [label]" into dest.
void parse_ifgoto(char *filename, Token *label, char *dest) {
    char code[MAX_LINE_LENGTH+200] = "";
    sprintf(code, "// If-goto\n@SP\n"
                  "M=M-1\n"
                  "A=M\n"
                  "D=M\n"
                  "@manual$%s$%s\n"
                  "D;JNE\n", filename, label->value.str_val);
    strcat(dest, code);
}

// Append assembly code to dest which loads the RAM address pointed to by [segment] [address] into A, where [segment]
// cannot be the keyword "constant".
void parse_load_data(char *filename, Token *segment, Token *address, char *dest) {
    char code[200] = "";

    switch (segment->value.key_val) {
        case LOCAL:
            sprintf(code, "@%d\n"
                          "D=A\n"
                          "@LCL\n"
                          "A=M+D\n", address->value.int_val);
            break;
        case ARGUMENT:
            sprintf(code, "@%d\n"
                          "D=A\n"
                          "@ARG\n"
                          "A=M+D\n", address->value.int_val);
            break;
        case KW_THIS:
            sprintf(code, "@%d\n"
                          "D=A\n"
                          "@THIS\n"
                          "A=M+D\n", address->value.int_val);
            break;
        case THAT:
            sprintf(code, "@%d\n"
                          "D=A\n"
                          "@THAT\n"
                          "A=M+D\n", address->value.int_val);
            break;
        case POINTER:
            if (address->value.int_val == 0) {
                strcat(code, "@THIS\n");
            } else {
                strcat(code, "@THAT\n");
            }
            break;
        case TEMP:
            sprintf(code,"@R%d\n", 5 + address->value.int_val);
            break;
        case STATIC:
            sprintf(code,"@%s.%d\n", filename, address->value.int_val);
            break;
        case CONSTANT:
            printf("Error: CONSTANT passed to parse_load_data.");
            exit(EXIT_FAILURE);
        default:
            printf("Malformed instruction!");
            exit(EXIT_FAILURE);
    }
    strcat(dest,code);
}

void parse_call(char *filename, Token *name, Token *args, char *dest) {
    char code[500+4*MAX_LINE_LENGTH] = "";
    char return_label[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, return_label);
    char function_label[MAX_LINE_LENGTH] = "";
    get_function_label(name->value.str_val, function_label);
    sprintf(code, "// Call\n"
                  "@%s // Push call frame to stack\n"
                  "D=A\n"
                  "@SP\n"
                  "A=M\n"
                  "M=D\n"

                  "@LCL\n"
                  "D=M\n"
                  "@SP\n"
                  "A=M+1\n"
                  "M=D\n"

                  "@ARG\n"
                  "D=M\n"
                  "@SP\n"
                  "A=M+1\n"
                  "A=A+1\n"
                  "M=D\n"

                  "@THIS\n"
                  "D=M\n"
                  "@SP\n"
                  "A=M+1\n"
                  "A=A+1\n"
                  "A=A+1\n"
                  "M=D\n"

                  "@THAT\n"
                  "D=M\n"
                  "@SP\n"
                  "A=M+1\n"
                  "A=A+1\n"
                  "A=A+1\n"
                  "A=A+1\n"
                  "M=D \n"

                  "D=A+1 // Update LCL\n"
                  "@LCL\n"
                  "M=D\n"

                  "@SP // Update ARG\n"
                  "D=M\n"
                  "@%d\n"
                  "D=D-A\n"

                  "@ARG\n"
                  "M=D\n"
                  "@%s // Jump to function\n"
                  "0;JMP\n"
                  "(%s) // Return label\n", return_label, args->value.int_val, function_label, return_label);
    strcat(dest, code);
}

void parse_function(char *filename, Token *name, Token *local_vars, char *dest) {
    char code[500+4*MAX_LINE_LENGTH] = "";
    char loop_start_label[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, loop_start_label);
    char loop_end_label[MAX_LINE_LENGTH] = "";
    get_next_label_name(filename, loop_end_label);
    char function_label[MAX_LINE_LENGTH] = "";
    get_function_label(name->value.str_val, function_label);
    sprintf(code, "// Function\n"
                  "(%s) // Function label\n"
                  "@R13 // Initialise local segment via loop\n"
                  "M=0  // (NB this is horribly slow, especially for small local segments!)\n"
                  "(%s) // Here R13 stores the i for which we're initialising local i\n"
                  "@R13\n"
                  "D=M\n"
                  "@%d\n"
                  "D=D-A\n"
                  "@%s\n"
                  "D;JEQ\n"
                  "@R13\n"
                  "D=M\n"
                  "M=M+1\n"
                  "@LCL\n"
                  "A=M+D\n"
                  "M=0\n"
                  "@%s\n"
                  "0;JMP\n"
                  "(%s)\n"
                  "@%d // Set SP\n"
                  "D=A\n"
                  "@LCL\n"
                  "D=M+D\n"
                  "@SP\n"
                  "M=D\n", function_label, loop_start_label, local_vars->value.int_val, loop_end_label,
            loop_start_label, loop_end_label, local_vars->value.int_val);
    strcat(dest, code);
}

void parse_return(char *dest) {
    strcat(dest, "// Return\n"
                 "@5 // Store return address in R13\n"
                 "D=A\n"
                 "@LCL \n"
                 "A=M-D\n"
                 "D=M\n"
                 "@R13\n"
                 "M=D\n"
                 "@SP // Copy return value to argument 0, NB this may overwrite\n"
                 "A=M-1 // return address if argument has length 0\n"
                 "D=M\n"
                 "@ARG\n"
                 "A=M\n"
                 "M=D\n"
                 "D=A+1 // Update SP\n"
                 "@SP\n"
                 "M=D\n"
                 "@LCL // Restore THAT\n"
                 "A=M-1\n"
                 "D=M\n"
                 "@THAT\n"
                 "M=D\n"
                 "@LCL // Restore THIS\n"
                 "A=M-1\n"
                 "A=A-1\n"
                 "D=M\n"
                 "@THIS\n"
                 "M=D\n"
                 "@3 // Restore ARG\n"
                 "D=A\n"
                 "@LCL\n"
                 "A=M-D\n"
                 "D=M\n"
                 "@ARG\n"
                 "M=D\n"
                 "@4 // Restore LCL\n"
                 "D=A\n"
                 "@LCL\n"
                 "A=M-D\n"
                 "D=M\n"
                 "@LCL\n"
                 "M=D\n"
                 "@R13 // Jump to return address\n"
                 "A=M\n"
                 "0;JMP\n");
}

// Puts a label name of the form auto$[filename]$[number] into dest, where [number] is unique to the file.
// Labels with names specified directly in VM code are translated into the form manual$[filename]$[label_name], so
// there's no danger of duplication.
void get_next_label_name(char *filename, char *dest) {
    static int label_count = 0;
    char buffer[MAX_LINE_LENGTH] = "";
    sprintf(buffer, "auto$%s$%d", filename, label_count);
    strcat(dest, buffer);
    label_count++;
}

// Puts a label name of the form call$[function_name] into dest. Recall that every function name in file Blah.vm
// has to start with "Blah.", so there can't be two functions with the same name anywhere in the folder and this
// label name will be unique across all the code we're compiling. 
void get_function_label(char *function_name, char *dest) {
    sprintf(dest, "call$%s", function_name);
}