#include <stdlib.h>
#include <string.h>
#include "symboltable.h"

SymbolTable *malloc_table() {
    const int initial_space = 1;

    SymbolTable *table = (SymbolTable*)malloc(sizeof(SymbolTable));
    table->table_length = 0;
    table->table_space = initial_space;
    table->table_array = (TableEntry**)malloc(initial_space * sizeof(TableEntry *));
    return table;
}

void free_table(SymbolTable *table) {
    for (int i=0; i<table->table_length; i++) {
        free(table->table_array[i]->name);
        free(table->table_array[i]);
    }
    free(table->table_array);
    free(table);
}

void add_to_table(SymbolTable *table, const char *name, int address) {
    TableEntry *new = malloc(sizeof(TableEntry));
    new->address = address;
    new->name = malloc(strlen(name)+1);
    strcpy(new->name, name);
    table->table_array[table->table_length] = new;
    (table->table_length)++;

    // If our table_array array is now full, allocate more memory.
    if (table->table_length == table->table_space) {
        TableEntry **new_array = (TableEntry**)malloc(2 * (table->table_length) * sizeof(TableEntry *));
        for (int i=0;i<table->table_length; i++) {
            new_array[i] = (table->table_array)[i];
        }
        free(table->table_array);
        table->table_array = new_array;
        table->table_space *= 2;
    }
}

int get_table_entry(const SymbolTable *table, const char *search_name) {
    // NOTE: The *right* way to do this is to use a hash map, which will runs in (amortised) O(1) time rather than
    // O(table->table_size) time. But this unit is already introducing a lot, and non-C languages include built-in hash
    // tables anyway (e.g. Java's HashMap and Python's dictionary), so let's keep it simple.
    for (int i=0; i<table->table_length; i++) {
        if (strcmp(table->table_array[i]->name, search_name) == 0) {
            return i;
        }
    }
    return -1;
}
