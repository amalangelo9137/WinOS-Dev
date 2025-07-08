#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Utility function to allocate memory and check for success
void* allocate_memory(size_t size) {
    void* ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

// Utility function to copy a string
char* copy_string(const char* str) {
    size_t len = strlen(str);
    char* new_str = (char*)allocate_memory(len + 1);
    strcpy(new_str, str);
    return new_str;
}

// Utility function to free allocated memory
void free_memory(void* ptr) {
    free(ptr);
}