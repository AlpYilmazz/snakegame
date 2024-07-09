#ifndef SNAKEGAME_SNAKE
#define SNAKEGAME_SNAKE

#include "stdlib.h"
#include "stdbool.h"

#include "raylib.h"

#include "grid.h"

typedef struct {
    Cell head;
    int tail_len;
    Cell tail[400]; // TODO: into dynamic array
    Cell tail_drag;
    Movement momentum;
    Color head_color;
    Color head_color_gameover_cycle;
    Color tail_color;
} Snake;

Snake new_snake(Cell head, Movement momentum);
int snake_len(Snake* snake);
bool move_snake(Snake* snake, CellVector move);
void eat_food(Snake* snake);
int does_intersect(Snake* snake);
int occupies_cell(Snake* snake, Cell cell);
Cell spawn_food(Snake* snake);

void snake_draw_system(Snake* snake, int head_gameover_cycle);

#endif