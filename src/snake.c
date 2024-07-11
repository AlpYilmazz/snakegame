#include <stdbool.h>

#include "raylib.h"
#include "raymath.h"

#include "util.h"
#include "grid.h"

#include "snake.h"

extern Grid GRID;
extern GridDimentions GRID_DIM;
extern GridWalls GRID_WALLS;

Snake new_snake(Cell head, Movement momentum) {
    return (Snake) {
        .head = head,
        .tail_len = 0,
        .tail = {0},
        .tail_drag = {0},
        .momentum = momentum,
        .head_color = GREEN,
        .head_color_gameover_cycle = RED,
        .tail_color = DARKGREEN,
    };
}

int snake_len(Snake* snake) {
    return snake->tail_len + 1;
}

bool move_snake(Snake* snake, CellVector move) {
    int row = snake->head.y;
    int wall_v = snake->head.x + __max(move.x, 0);

    int col = snake->head.x;
    int wall_h = snake->head.y + __max(move.y, 0);

    if (move.x != 0 && GRID_WALLS.vertical[row][wall_v]) {
        return false;
    }
    if (move.y != 0 && GRID_WALLS.horizontal[col][wall_h]) {
        return false;
    }

    Cell follow = snake->head;
    snake->head = cell_add_wrapping(snake->head, move);
    for (int tail_i = 0; tail_i < snake->tail_len; tail_i++) {
        Cell follow_temp = snake->tail[tail_i];
        snake->tail[tail_i] = follow;
        follow = follow_temp;
    }
    snake->tail_drag = follow;
    return true;
}

void eat_food(Snake* snake) {
    snake->tail[snake->tail_len] = snake->tail_drag;
    snake->tail_len++;
}

int does_intersect(Snake* snake) {
    for (int tail_i = 0; tail_i < snake->tail_len; tail_i++) {
        if (cell_equals(snake->head, snake->tail[tail_i])) {
            return 1;
        }
    }
    return 0;
}

int occupies_cell(Snake* snake, Cell cell) {
    if (cell_equals(cell, snake->head)) {
        return 1;
    }
    for (int tail_i = 0; tail_i < snake->tail_len; tail_i++) {
        if (cell_equals(cell, snake->tail[tail_i])) {
            return 1;
        }
    }
    return 0;
}

Cell spawn_food(Snake* snake) {
    Cell food_cell;
    do {
        food_cell = (Cell) {
            .x = rand() % GRID_DIM.COLS,
            .y = rand() % GRID_DIM.ROWS,
        };
    } while(occupies_cell(snake, food_cell));
    return food_cell;
}

void snake_draw_system(Snake* snake, int head_gameover_cycle) {
    for (int tail_i = 0; tail_i < snake->tail_len; tail_i++) {
        Cell snake_tail = snake->tail[tail_i];
        float tail_size = (float)GRID.CONTENT_CELL_SIZE * 0.7;
        float tail_margin = (GRID.CONTENT_CELL_SIZE - tail_size)/2.0;
        Rectangle snake_tail_content = {
            .x = snake_tail.x * GRID.CELL_SIZE + tail_margin + GRID.CONTENT_MARGIN,
            .y = snake_tail.y * GRID.CELL_SIZE + tail_margin + GRID.CONTENT_MARGIN,
            .width = tail_size,
            .height = tail_size,
        };
        DrawRectangleRec(snake_tail_content, snake->tail_color);
    }
    
    Rectangle snake_head_content = {
        .x = snake->head.x * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
        .y = snake->head.y * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
        .width = GRID.CONTENT_CELL_SIZE,
        .height = GRID.CONTENT_CELL_SIZE,
    };
    Color snake_head_color = (head_gameover_cycle == 0)
                            ? snake->head_color
                            : snake->head_color_gameover_cycle;
    DrawRectangleRec(snake_head_content, snake_head_color);
}