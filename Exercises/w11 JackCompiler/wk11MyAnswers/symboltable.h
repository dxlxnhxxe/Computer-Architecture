// NOTE: This code is currently untested, caveat emptor!
#include <stdbool.h>

// Each entry in our symbol table consists of an integer offset, a variable name, a variable type (which could be a
// primitive type or a class_table name), and a "kind" denoting how the variable was defined --- as a local variable with a
// var statement (VK_VAR), a subroutine_table argument (VG_ARGUMENT), a class_table field (VK_FIELD) or a class_table static variable
// (VK_STATIC).
enum VariableKind {
    VK_VAR, VK_ARGUMENT, VK_FIELD, VK_STATIC
}; typedef enum VariableKind VariableKind;
#define NUM_KINDS 4

struct TableEntry {
    char *name;
    char *type;
    VariableKind kind;
    int offset;
}; typedef struct TableEntry TableEntry;

// A SymbolTable is a collection of TableEntries stored in table_array, with the number of entries
// stored in table_length. Every entry is required to have a unique name. You can add an unlimited
// number of entries to the table, and you can search for an entry by its name. num_allocated[kind]
// contains the number of variables of the given kind stored in the table. The table_space
// variable is only used internally in adding new items to the table.
struct SymbolTable {
    TableEntry **table_array; // Remember a double pointer is the same as an array of pointers.
    int table_length;
    int table_space;
    int num_allocated[NUM_KINDS];
}; typedef struct SymbolTable SymbolTable;

// Creates and returns a new empty symbol table.
SymbolTable *malloc_table();
// Returns true if the given table entry is for a primitive type (int, char, or boolean).
bool is_primitive(TableEntry *entry);
// Frees every entry in table, then table itself.
void free_table(SymbolTable *table);
// Adds a new entry to table with the given name, type and kind. Automatically assigns the first unallocated offset
// among variables of that kind. Note that name and type will be assigned by value, not reference.
void add_to_table(SymbolTable *table, const char *name, const char *type, VariableKind kind);
// If table contains an entry with name search_name, returns i such that table->table_array[i] is
// that entry. If table doesn't contain such an entry, returns -1.
int get_table_entry(const SymbolTable *table, const char *search_name);

