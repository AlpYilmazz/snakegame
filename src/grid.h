#ifndef SNAKEGAME_GRID
#define SNAKEGAME_GRID

#include "raylib.h"

#define MAX_GRID_DIMENTION 20
#define MAX_CELL_COUNT 400

typedef struct {
    int CELL_SIZE;
    int LINE_THICKNESS;
    int CONTENT_MARGIN;
    int CONTENT_CELL_SIZE;
    int WALL_THICKNESS;
} Grid;

typedef struct {
    int ROWS;
    int COLS;
    int HEIGHT;
    int WIDTH;
} GridDimentions;

typedef struct {
    int vertical[MAX_GRID_DIMENTION][MAX_GRID_DIMENTION];   // [row_i][col_i], [row_i][col_i + 1] -> [0][0], [0][1]
    int horizontal[MAX_GRID_DIMENTION][MAX_GRID_DIMENTION]; // [col_i][row_i], [col_i][row_i + 1]
} GridWalls;

typedef struct {
    Vector2 cell_position[MAX_GRID_DIMENTION][MAX_GRID_DIMENTION];
    Vector2 cell_velocity[MAX_GRID_DIMENTION][MAX_GRID_DIMENTION];
} GridCellObjects;

typedef struct {
    int x;
    int y;
} Cell;

typedef struct {
    int x;
    int y;
} CellVector;

typedef enum {
    NONE, UP, DOWN, RIGHT, LEFT
} Movement;

Movement movement_negate(Movement move);
CellVector get_movement_direction(Movement move);

int cell_equals(Cell this, Cell other);
Cell cell_add_wrapping(Cell cell, CellVector add);

#endif