#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util.h"

int rand_in_range(int l, int r) {
    return l + (rand() % (r - l + 1));
}

float rand_float_discrete(int l, int r, int point_step) {
    float base = (float)rand_in_range(l, r-1);
    float fac = (float)rand_in_range(0, point_step);
    float fl = fac / (float)point_step;
    return base + fl;
}

StringList StringList_empty() {
    return (StringList) {0};
}

void StringList_push_back(StringList* list, char* item) {
    #define INITIAL_CAPACITY 10
    #define CAPACITY_GROWTH_FACTOR 2
    
    if (list->capacity == 0) {
        list->capacity = INITIAL_CAPACITY;
        list->items = malloc(INITIAL_CAPACITY * sizeof(*&item));
    }
    else if (list->count == list->capacity) {
        int new_capacity = CAPACITY_GROWTH_FACTOR * list->capacity;
        list->items = realloc(list->items, new_capacity * sizeof(*&item));
    }

    list->items[list->count] = item;
    list->count++;

    #undef INITIAL_CAPACITY
    #undef CAPACITY_GROWTH_FACTOR
}

// TODO: error handling
StringList list_files_in_directory(const char* dir) {
    StringList files = StringList_empty();

    WIN32_FIND_DATA file_find_data;
    HANDLE file_handle = NULL;
    char path_spec[2048];

    sprintf(path_spec, "%s\\*.*", dir);

    if((file_handle = FindFirstFile(path_spec, &file_find_data)) == INVALID_HANDLE_VALUE) {
        printf("Path not found: [%s]\n", dir);
        return files;
    }

    do {
        // Find first file will always return "."
        // and ".." as the first two directories.
        if(strcmp(file_find_data.cFileName, ".") == 0 || strcmp(file_find_data.cFileName, "..") == 0) {
            continue;
        }
        // Build up our file path using the passed in
        // [sDir] and the file/foldername we just found:
        sprintf(path_spec, "%s\\%s", dir, file_find_data.cFileName);

        // Is the entity a File or Folder?
        if(!(file_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            char* filename = malloc(strlen(path_spec) + 1);
            strcpy(filename, path_spec);
            StringList_push_back(&files, filename);
        }
    } while(FindNextFile(file_handle, &file_find_data)); // Find the next file.

    FindClose(file_handle);

    return files;
}

static unsigned char buffer[500000] = {0}; // 300 x 300 x 4 = 360_000
GenericData read_file_data(const char* filename) {
    FILE* file = fopen(filename, "rb");

    unsigned int count = 0;
    int ch;
    while((ch = fgetc(file)) != EOF) {
        buffer[count] = ch;
        count++;
    }
    printf("[LOAD] ch: %d", ch);
    fclose(file);

    printf("[LOAD] file: %s, size: %d\n", filename, count);
    GenericData generic_data = {
        .size_in_bytes = count,
        .data = malloc(count),
    };
    memcpy(generic_data.data, &buffer[0], count);

    return generic_data;
}