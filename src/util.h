#ifndef SNAKEGAME_UTIL
#define SNAKEGAME_UTIL

#include <stdlib.h>
#include <stdbool.h>

int rand_in_range(int l, int r);
float rand_float_discrete(int l, int r, int point_step);

typedef struct {
    unsigned int count;
    unsigned int capacity;
    char** items;
} StringList;

StringList StringList_empty();
void StringList_push_back(StringList* list, char* item);
StringList list_files_in_directory(const char* dir);

typedef struct {
    unsigned int size_in_bytes;
    unsigned char* data;
} GenericData;

GenericData read_file_data(const char* filename);

#endif