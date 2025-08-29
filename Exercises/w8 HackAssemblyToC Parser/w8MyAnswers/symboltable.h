// Each entry in a symbol table consists of an integer memory address and an identifier name
// (for e.g. a variable or label).
struct TableEntry {
    char *name;
    int address;
}; typedef struct TableEntry TableEntry;

// A table is a collection of TableEntries stored in table_array, with the number of entries
// stored in table_length. Every entry is required to have a unique name. You can add an unlimited
// number of entries to the table, and you can search for an entry by its name. The table_space
// variable is only used internally and tracks the memory allocated to table_array.
struct SymbolTable {
    TableEntry **table_array; // Remember a double pointer is the same as an array of pointers.
    int table_length;
    int table_space;
}; typedef struct SymbolTable SymbolTable;

// Creates and returns a new empty symbol table.
SymbolTable *malloc_table();
// Frees every entry in table, then table itself.
void free_table(SymbolTable *table);
// Adds a new entry to table with the given name and address.
void add_to_table(SymbolTable *table, const char *name, int address);
// If table contains an entry with name search_name, returns i such that table->table_array[i] is
// that entry. If table doesn't contain such an entry, returns -1.
int get_table_entry(const SymbolTable *table, const char *search_name);

