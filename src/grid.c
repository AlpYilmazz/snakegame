#include "raylib.h"

#include "grid.h"

extern Grid GRID;
extern GridDimentions GRID_DIM;

Movement movement_negate(Movement move) {
    switch (move) {
        case NONE:
            return NONE;
        case UP:
            return DOWN;
        case DOWN:
            return UP;
        case RIGHT:
            return LEFT;
        case LEFT:
            return RIGHT;
    }
}

CellVector get_movement_direction(Movement move) {
    switch (move) {
        case NONE:
            return (CellVector) { .x = 0.0, .y = 0.0 };
        case UP:
            return (CellVector) { .x = 0.0, .y = -1.0 };
        case DOWN:
            return (CellVector) { .x = 0.0, .y = 1.0 };
        case RIGHT:
            return (CellVector) { .x = 1.0, .y = 0.0 };
        case LEFT:
            return (CellVector) { .x = -1.0, .y = 0.0 };
    }
}

int cell_equals(Cell this, Cell other) {
    return (this.x == other.x) && (this.y == other.y);
}

Cell cell_add_wrapping(Cell cell, CellVector add) {
    return (Cell) {
        .x = (cell.x + GRID_DIM.COLS + add.x) % GRID_DIM.COLS,
        .y = (cell.y + GRID_DIM.ROWS + add.y) % GRID_DIM.ROWS,
    };
}
