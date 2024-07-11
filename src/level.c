#include <stdio.h>

#include "grid.h"

#include "level.h"

void save_level(int level_index, GameLevel* level) {
    char filename[50] = {0};
    sprintf(filename, "./levels/%d.lvl", level_index);
    FILE* file = fopen(filename, "w");

    fprintf(file, "%d %d\n", level->rows, level->cols);
    fprintf(file, "%d %d\n", level->snake_entry_cell.x, level->snake_entry_cell.y);
    fprintf(file, "%d\n", level->snake_entry_momentum);
    for (int i = 0; i < level->rows; i++) {
        fprintf(file, "%d", level->vertical_walls[i][0]);
        for (int j = 1; j < level->cols + 1; j++) {
            fprintf(file, " %d", level->vertical_walls[i][j]);
        }
        fprintf(file, "\n");
    }
    for (int i = 0; i < level->cols; i++) {
        fprintf(file, "%d", level->horizontal_walls[i][0]);
        for (int j = 1; j < level->rows + 1; j++) {
            fprintf(file, " %d", level->horizontal_walls[i][j]);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}

GameLevel load_level(int level_index) {
    GameLevel level = {0};

    char filename[50] = {0};
    sprintf(filename, "./levels/%d.lvl", level_index);
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        printf("File could not be opened for reading");
        level = (GameLevel) {
            .rows = 15,
            .cols = 10,
            .snake_entry_cell = (Cell) { 0, 7 },
            .snake_entry_momentum = RIGHT,
            .vertical_walls = {0},
            .horizontal_walls = {0},
        };
        save_level(level_index, &level);
        return level;
    }

    fscanf(file, "%d %d\n", &level.rows, &level.cols);
    fscanf(file, "%d %d\n", &level.snake_entry_cell.x, &level.snake_entry_cell.y);
    fscanf(file, "%d\n", &level.snake_entry_momentum);
    for (int i = 0; i < level.rows; i++) {
        for (int j = 0; j < level.cols + 1; j++) {
            fscanf(file, "%d", &level.vertical_walls[i][j]);
        }
    }
    for (int i = 0; i < level.cols; i++) {
        for (int j = 0; j < level.rows + 1; j++) {
            fscanf(file, "%d", &level.horizontal_walls[i][j]);
        }
    }

    fclose(file);
    return level;
}