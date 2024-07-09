#ifndef SNAKEGAME_LEVEL
#define SNAKEGAME_LEVEL

#include "grid.h"

typedef struct {
    int rows;
    int cols;
    Cell snake_entry_cell;
    Movement snake_entry_momentum;
    int vertical_walls[20][20];
    int horizontal_walls[20][20];
} GameLevel;

void save_level(int level_index, GameLevel* level);
GameLevel load_level(int level_index);

#endif