#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "threadpool.h"

#include "util.h"
#include "asyncio.h"
#include "grid.h"
#include "level.h"
#include "asset.h"
#include "animation.h"
#include "gameui.h"
#include "snake.h"
#include "fireworks.h"

// -- GLOBAL VARIABLE TYPES -----
typedef enum {
    // INIT, PAUSE,
    PLAYING,
    GAMEOVER_LOSE,
    GAMEOVER_WIN,
} GameState;
// ---------------------

// -- GLOBAL VARIABLES -----
const int SCREEN_WIDTH = 1800;
const int SCREEN_HEIGHT = 1000;

ThreadPool* THREAD_POOL;
TextureAssets TEXTURE_ASSETS;

Grid GRID = {0};
GridDimentions GRID_DIM = {0};
GridWalls GRID_WALLS = {0};
GridCellObjects GRID_CELL_OBJECTS = {0};

GameState GAME_STATE = {0};

void init_grid_dimentions(int rows, int cols) {
    GRID_DIM = (GridDimentions) {
        .ROWS = rows,
        .COLS = cols,
        .HEIGHT = rows * GRID.CELL_SIZE,
        .WIDTH = cols * GRID.CELL_SIZE,
    };
}

int total_cell_count() {
    return GRID_DIM.ROWS * GRID_DIM.COLS;
}

int is_game_over() {
    return GAME_STATE == GAMEOVER_LOSE || GAME_STATE == GAMEOVER_WIN;
}
// -------------------------

void grid_draw_system() {
    Rectangle grid_frame = {
        .x = 0.0,
        .y = 0.0,
        .width = GRID_DIM.WIDTH,
        .height = GRID_DIM.HEIGHT,
    };

    DrawRectangleLinesEx(grid_frame, GRID.LINE_THICKNESS, BLUE);
    DrawRectangleRec(grid_frame, BEIGE);

    for (int i = 0; i < GRID_DIM.ROWS; i++) {
        for (int j = 0; j < GRID_DIM.COLS; j++) {
            Rectangle cell = {
                .x = j * GRID.CELL_SIZE,
                .y = i * GRID.CELL_SIZE,
                .width = GRID.CELL_SIZE,
                .height = GRID.CELL_SIZE,
            };
            DrawRectangleLinesEx(cell, GRID.LINE_THICKNESS/2, DARKGRAY);
        }
    }
}

void grid_walls_draw_system(int* last_hovered_wall) {
    for (int i = 0; i < GRID_DIM.ROWS; i++) {
        for (int w = 0; w < GRID_DIM.COLS+1; w++) {
            int* wall = &GRID_WALLS.vertical[i][w];
            Rectangle wall_rect = {
                .x = (w * GRID.CELL_SIZE) - GRID.WALL_THICKNESS/2,
                .y = (i * GRID.CELL_SIZE),// + GRID.WALL_THICKNESS/2,
                .width = GRID.WALL_THICKNESS,
                .height = GRID.CELL_SIZE,// - GRID.WALL_THICKNESS,
            };
            Color wall_color = BLUE;
            if (*wall) {
                if (wall == last_hovered_wall) {
                    wall_color = RED;
                }
                DrawRectangleRec(wall_rect, wall_color);
            }
            if (wall == last_hovered_wall) {
                wall_color.a = 120;
                DrawRectangleRec(wall_rect, wall_color);
            }
        }
    }
    for (int i = 0; i < GRID_DIM.COLS; i++) {
        for (int w = 0; w < GRID_DIM.ROWS+1; w++) {
            int* wall = &GRID_WALLS.horizontal[i][w];
            Rectangle wall_rect = {
                .x = (i * GRID.CELL_SIZE),// + GRID.WALL_THICKNESS/2,
                .y = (w * GRID.CELL_SIZE) - GRID.WALL_THICKNESS/2,
                .width = GRID.CELL_SIZE,// - GRID.WALL_THICKNESS,
                .height = GRID.WALL_THICKNESS,
            };
            Color wall_color = BLUE;
            if (*wall) {
                if (wall == last_hovered_wall) {
                    wall_color = RED;
                }
                DrawRectangleRec(wall_rect, wall_color);
            }
            if (wall == last_hovered_wall) {
                wall_color.a = 150;
                DrawRectangleRec(wall_rect, wall_color);
            }
        }
    }
}

void grid_cell_objects_draw_system() {
    for (int i = 0; i < GRID_DIM.ROWS; i++) {
        for (int j = 0; j < GRID_DIM.COLS; j++) {
            Vector2 pos = GRID_CELL_OBJECTS.cell_position[i][j];
            Rectangle cell = {
                .x = pos.x,
                .y = pos.y,
                .width = GRID.CELL_SIZE,
                .height = GRID.CELL_SIZE,
            };
            DrawRectangleRec(cell, BEIGE);

            Rectangle cell_frame = {
                .x = cell.x - GRID.LINE_THICKNESS/2,
                .y = cell.y - GRID.LINE_THICKNESS/2,
                .width = GRID.CELL_SIZE + GRID.LINE_THICKNESS,
                .height = GRID.CELL_SIZE + GRID.LINE_THICKNESS,
            };
            DrawRectangleLinesEx(cell_frame, GRID.LINE_THICKNESS, DARKGRAY);
        }
    }
}

void grid_cell_objects_position_update_system(float delta_time_seconds) {
    const float DRAG_COEFF = 1.0 * 2.0;

    for (int i = 0; i < GRID_DIM.ROWS; i++) {
        for (int j = 0; j < GRID_DIM.COLS; j++) {
            Vector2 velocity = GRID_CELL_OBJECTS.cell_velocity[i][j];

            const Vector2 neg_velocity_norm = Vector2Normalize(Vector2Negate(velocity));
            const float v_len = Vector2Length(velocity);
            const float v_sq = v_len; // * v_len;
            const float cut_section = 1;

            const Vector2 AIR_DRAG = Vector2Scale(
                neg_velocity_norm,
                DRAG_COEFF * (v_sq/2) * cut_section
            );
            Vector2 accelaration = AIR_DRAG;

            GRID_CELL_OBJECTS.cell_velocity[i][j] = Vector2Add(
                velocity,
                Vector2Scale(accelaration, delta_time_seconds)
            );
            if (Vector2LengthSqr(GRID_CELL_OBJECTS.cell_velocity[i][j]) < EPSILON * EPSILON) {
                GRID_CELL_OBJECTS.cell_velocity[i][j] = Vector2Zero();
            }

            GRID_CELL_OBJECTS.cell_position[i][j] = Vector2Add(
                GRID_CELL_OBJECTS.cell_position[i][j],
                Vector2Scale(GRID_CELL_OBJECTS.cell_velocity[i][j], delta_time_seconds)
            );
        }
    }
}

void mock_win_by_space_press_system() {
    if (IsKeyPressed(KEY_SPACE)) {
        GAME_STATE = GAMEOVER_WIN;
    }
}

FireworksResources FIREWORKS_WINSCREEN;

void init_fireworks() {
    Fireworks fireworks_1 = {
        // .origin = Vector2Zero(),
        // .origin = (Vector2) {
        //     .x = 5.0,
        //     .y = 5.0,
        // },
        .origin = (Vector2) {
            .x = (GRID.CELL_SIZE * GRID_DIM.COLS)/2.0,
            .y = (GRID.CELL_SIZE * (GRID_DIM.ROWS + 10))/2.0,
        },
        .spawn_radius_max = 50.0,
        .spawn_radius_min = 10.0,
        .start_angle = PI/4.0,
        .end_angle = 3*PI/4.0,
        .angle_increment = PI/40,
        .particles_per_angle = 10,

        .particle_speed = 400.0,
        .particle_ttl = 2.0,
        .particle_spawn_size = 20.0,

        .particle_color = RED,
    };

    Fireworks fireworks_2 = fireworks_1;
    fireworks_2.origin = (Vector2) {
        .x = (GRID.CELL_SIZE * (GRID_DIM.COLS + 6))/2.0,
        .y = (GRID.CELL_SIZE * (GRID_DIM.ROWS + 16))/2.0,
    };
    fireworks_2.particle_speed = 700.0;
    fireworks_2.particle_spawn_size = 30.0;
    fireworks_2.particle_color = BLUE;

    Fireworks fireworks_3 = fireworks_1;
    fireworks_3.origin = (Vector2) {
        .x = (GRID.CELL_SIZE * (GRID_DIM.COLS - 6))/2.0,
        .y = (GRID.CELL_SIZE * (GRID_DIM.ROWS + 26))/2.0,
    };
    fireworks_3.particle_speed = 700.0;
    fireworks_3.particle_ttl = 4.0;
    fireworks_3.particle_color = MAGENTA;

    int sequence_len = 4;
    float sq[4] = { 0.5, 1.2, 1.5, 3.0 };
    float* sequence = malloc(sequence_len * sizeof(float));
    memcpy(sequence, &sq, sequence_len * sizeof(float));
    SequenceTimer fireworks_timer = new_sequence_timer(sequence, sequence_len, Timer_Repeating);

    const int fireworks_count = 3;
    FIREWORKS_WINSCREEN = (FireworksResources) {
        .fireworks_started = false,
        .fireworks = malloc(fireworks_count * sizeof(Fireworks)),
        .fireworks_count = fireworks_count,
        .fireworks_index = 0,
        .fireworks_timer = fireworks_timer,
    };
    FIREWORKS_WINSCREEN.fireworks[0] = fireworks_1;
    FIREWORKS_WINSCREEN.fireworks[1] = fireworks_2;
    FIREWORKS_WINSCREEN.fireworks[2] = fireworks_3;
}

void edit_level(GameLevel* level) {

    init_grid_dimentions(level->rows, level->cols);
    for (int i = 0; i < level->rows; i++) {
        for (int j = 0; j < level->cols + 1; j++) {
            GRID_WALLS.vertical[i][j] = level->vertical_walls[i][j];
        }
    }
    for (int i = 0; i < level->cols; i++) {
        for (int j = 0; j < level->rows + 1; j++) {
            GRID_WALLS.horizontal[i][j] = level->horizontal_walls[i][j];
        }
    }

    Camera2D camera = {0};
    camera.zoom = 1.0;
    camera.offset = (Vector2) {
        .x = (SCREEN_WIDTH - GRID_DIM.WIDTH)/2.0,
        .y = (SCREEN_HEIGHT - GRID_DIM.HEIGHT)/2.0,
    };

    Vector2 button_size = { 300, 100 };
    Vector2 sq_small_button_size = { 50, 50 };
    Vector2 button_margin = Vector2Scale(button_size, 0.25);
    // Vector2 first_button_position = {
    //     .x = 15,
    //     .y = SCREEN_HEIGHT/2 - (button_count * button_size.y + (button_count-1) * button_margin_y)/2,
    // };
    Color button_idle_color = WHITE;
    Color button_hover_color = YELLOW;
    Color button_clicked_color = ORANGE;
    Color button_disabled_color = GRAY;
    Color button_active_color = GREEN;

    float div_margin_x = button_margin.x;
    Vector2 buttons_div_size = {
        .x = 2*button_margin.x + button_size.x,
        .y = 4*sq_small_button_size.y + 5*button_margin.y,
    };
    Vector2 buttons_div_position = {
        .x = div_margin_x,
        .y = SCREEN_HEIGHT/2 - buttons_div_size.y/2,
    };

    Button update_button = (Button) {
        .title = "UPDATE",
        .position = (Vector2) {
            .x = buttons_div_position.x,
            .y = buttons_div_position.y + sq_small_button_size.y + button_margin.y,
        },
        .size = Vector2Scale(button_size, 0.5),
    };
    Button up_button = (Button) {
        .title = "U",
        .position = (Vector2) {
            .x = buttons_div_position.x + sq_small_button_size.x,
            .y = update_button.position.y + update_button.size.y + 2*button_margin.y,
        },
        .size = sq_small_button_size,
    };
    Button down_button = (Button) {
        .title = "D",
        .position = (Vector2) {
            .x = buttons_div_position.x + sq_small_button_size.x,
            .y = up_button.position.y + 2*sq_small_button_size.y,
        },
        .size = sq_small_button_size,
    };
    Button right_button = (Button) {
        .title = "R",
        .position = (Vector2) {
            .x = buttons_div_position.x + 2*sq_small_button_size.x,
            .y = up_button.position.y + sq_small_button_size.y,
        },
        .size = sq_small_button_size,
    };
    Button left_button = (Button) {
        .title = "L",
        .position = (Vector2) {
            .x = buttons_div_position.x,
            .y = up_button.position.y + sq_small_button_size.y,
        },
        .size = sq_small_button_size,
    };

    Button save_button = (Button) {
        .title = "SAVE",
        .position = (Vector2) {
            .x = buttons_div_position.x,
            .y = down_button.position.y + down_button.size.y + 2*button_margin.y,
        },
        .size = Vector2Scale(button_size, 0.5),
    };

    TextInput rows_text_input = (TextInput) {
        .title = "ROWS",
        .input = {0},
        .position = buttons_div_position,
        .size = sq_small_button_size,
    };
    TextInput cols_text_input = (TextInput) {
        .title = "COLS",
        .input = {0},
        .position = (Vector2) {
            .x = buttons_div_position.x + rows_text_input.size.x + button_margin.x/2,
            .y = buttons_div_position.y,
        },
        .size = sq_small_button_size,
    };

    sprintf(&rows_text_input.input[0], "%d", level->rows);
    sprintf(&cols_text_input.input[0], "%d", level->cols);

    int text_input_count = 2;
    TextInput text_inputs[2] = {
        rows_text_input,
        cols_text_input,
    };

    for (int i = 0; i < text_input_count; i++) {
        TextInput* text_input = &text_inputs[i];
        text_input->id = i;
        text_input->color = button_idle_color;
    }

    int button_count = 6;
    Button buttons[6] = {
        update_button,
        up_button,
        down_button,
        right_button,
        left_button,
        save_button,
    };

    for (int i = 0; i < button_count; i++) {
        Button* button = &buttons[i];
        button->id = i;
        button->color = button_idle_color;
    }

    Cell snake_entry_cell = level->snake_entry_cell;
    int active_direction = level->snake_entry_momentum;

    int active_text_input_id = -1;
    bool holding_snake_head = false;
    int* last_hovered_wall = NULL;

    bool editor_should_close = false;
    while (!editor_should_close && !WindowShouldClose()) {
        bool mouse_button_left_pressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        bool mouse_button_left_released = IsMouseButtonReleased(MOUSE_BUTTON_LEFT);

        buttons[UP].disabled = false;
        buttons[DOWN].disabled = false;
        buttons[RIGHT].disabled = false;
        buttons[LEFT].disabled = false;
        if (snake_entry_cell.x == 0) {
            buttons[LEFT].disabled = true;
            buttons[LEFT].color = button_disabled_color;
        }
        if (snake_entry_cell.x == GRID_DIM.COLS-1) {
            buttons[RIGHT].disabled = true;
            buttons[RIGHT].color = button_disabled_color;
        }
        if (snake_entry_cell.y == 0) {
            buttons[UP].disabled = true;
            buttons[UP].color = button_disabled_color;
        }
        if (snake_entry_cell.y == GRID_DIM.ROWS-1) {
            buttons[DOWN].disabled = true;
            buttons[DOWN].color = button_disabled_color;
        }

        if (buttons[active_direction].disabled) {
            active_direction = !buttons[UP].disabled ? UP
                            : !buttons[DOWN].disabled ? DOWN
                            : !buttons[RIGHT].disabled ? RIGHT
                            : !buttons[LEFT].disabled ? LEFT
                            : NONE; // should not be possible
        }

        int hovered_text_input_id = -1;
        for (int i = 0; i < text_input_count; i++) {
            if (TextInput_hovered(&text_inputs[i])) {
                hovered_text_input_id = i;
            }
        }

        int hovered_button_id = -1;
        for (int i = 0; i < button_count; i++) {
            if (buttons[i].disabled) {
                buttons[i].color = button_disabled_color;
            }
            else if (Button_hovered(&buttons[i])) {
                hovered_button_id = i;
                buttons[i].color = button_hover_color;
            }
            else if (i == active_direction) {
                buttons[i].color = button_active_color;
            }
            else {
                buttons[i].color = button_idle_color;
            }
        }

        if (mouse_button_left_pressed) {
            if (hovered_text_input_id != -1) {
                if (active_text_input_id != -1 && active_text_input_id != hovered_text_input_id) {
                    text_inputs[active_text_input_id].color = button_idle_color;
                }
                active_text_input_id = hovered_text_input_id;
                text_inputs[active_text_input_id].color = button_clicked_color;
            }
            else {
                if (active_text_input_id != -1) {
                    text_inputs[active_text_input_id].color = button_idle_color;
                }
                active_text_input_id = -1;
            }

            // if (hovered_button_id != -1) {
            //     buttons[hovered_button_id].color = button_clicked_color;
            // }
        }

        if (active_text_input_id != -1) {
            int key = GetKeyPressed();
            if ('0' <= key && key <= '9') {
                TextInput_input(&text_inputs[active_text_input_id], key);
            }
            if (key == KEY_BACKSPACE) {
                TextInput_delete_back(&text_inputs[active_text_input_id]);
            }
        }

        if (mouse_button_left_released) {
            if (hovered_button_id != -1) {
                // buttons[hovered_button_id].color = button_idle_color;

                switch (hovered_button_id) {
                case 0: // UPDATE GRID
                    int rows, cols;
                    sscanf(&text_inputs[0].input[0], "%d", &rows);
                    sscanf(&text_inputs[1].input[0], "%d", &cols);
                    init_grid_dimentions(rows, cols);
                    camera.offset = (Vector2) {
                        .x = (SCREEN_WIDTH - GRID_DIM.WIDTH)/2.0,
                        .y = (SCREEN_HEIGHT - GRID_DIM.HEIGHT)/2.0,
                    };
                    break;
                case UP: // UP
                case DOWN: // DOWN
                case RIGHT: // RIGHT
                case LEFT: // LEFT
                    if (buttons[hovered_button_id].disabled) {
                        break;
                    }
                    active_direction = hovered_button_id;
                    break;
                case 5: // SAVE
                    level->rows = GRID_DIM.ROWS;
                    level->cols = GRID_DIM.COLS;
                    level->snake_entry_cell = snake_entry_cell;
                    level->snake_entry_momentum = active_direction;

                    for (int i = 0; i < level->rows; i++) {
                        for (int j = 0; j < level->cols + 1; j++) {
                            level->vertical_walls[i][j] = GRID_WALLS.vertical[i][j];
                        }
                    }
                    for (int i = 0; i < level->cols; i++) {
                        for (int j = 0; j < level->rows + 1; j++) {
                            level->horizontal_walls[i][j] = GRID_WALLS.horizontal[i][j];
                        }
                    }

                    editor_should_close = true;
                    break;
                }
            }
        }

        Vector2 mouse = GetMousePosition();
        Vector2 mouse_world = GetScreenToWorld2D(mouse, camera);
        mouse_world.x += GRID.CELL_SIZE;
        mouse_world.y += GRID.CELL_SIZE;
        int row = (int)mouse_world.y / GRID.CELL_SIZE;
        int col = (int)mouse_world.x / GRID.CELL_SIZE;
        int row_u = ((int)mouse_world.y - 10) / GRID.CELL_SIZE;
        int row_d = ((int)mouse_world.y + 10) / GRID.CELL_SIZE;
        int col_l = ((int)mouse_world.x - 10) / GRID.CELL_SIZE;
        int col_r = ((int)mouse_world.x + 10) / GRID.CELL_SIZE;

        if (mouse_button_left_pressed && snake_entry_cell.x == col-1 && snake_entry_cell.y == row-1) {
            holding_snake_head = true;
        }
        if (mouse_button_left_released) {
            holding_snake_head = false;
        }

        if (0 <= row && row <= GRID_DIM.ROWS+1
            && 0 <= col && col <= GRID_DIM.COLS+1
        ) {
            if (holding_snake_head
                && 1 <= row && row <= GRID_DIM.ROWS
                && 1 <= col && col <= GRID_DIM.COLS
            ) {
                snake_entry_cell.x = col-1;
                snake_entry_cell.y = row-1;
            }
            else if (col < col_r
                && 1 <= row && row <= GRID_DIM.ROWS
                && col <= GRID_DIM.COLS
            ) {
                // wall: vertical[row][col+1]
                last_hovered_wall = &GRID_WALLS.vertical[row-1][col-1+1];
            }
            else if (col_l < col
                && 1 <= row && row <= GRID_DIM.ROWS
                && 1 <= col
            ) {
                // wall: vertical[row][col]
                last_hovered_wall = &GRID_WALLS.vertical[row-1][col-1];
            }
            else if (row < row_d
                && row <= GRID_DIM.ROWS
                && 1 <= col && col <= GRID_DIM.COLS
            ) {
                // wall: horizontal[col][row+1]
                last_hovered_wall = &GRID_WALLS.horizontal[col-1][row-1+1];
            }
            else if (row_u < row
                && 1 <= row
                && 1 <= col && col <= GRID_DIM.COLS
            ) {
                // wall: horizontal[col][row]
                last_hovered_wall = &GRID_WALLS.horizontal[col-1][row-1];
            }
            else if (last_hovered_wall != NULL) {
                last_hovered_wall = NULL;
            }
        }
        else {
            last_hovered_wall = NULL;
        }

        if (last_hovered_wall != NULL && mouse_button_left_pressed) {
            *last_hovered_wall = !*last_hovered_wall;
        }

        if (snake_entry_cell.x == 0) {
            buttons[LEFT].disabled = true;
            buttons[LEFT].color = button_disabled_color;
        }
        if (snake_entry_cell.x == GRID_DIM.COLS-1) {
            buttons[RIGHT].disabled = true;
            buttons[RIGHT].color = button_disabled_color;
        }
        if (snake_entry_cell.y == 0) {
            buttons[UP].disabled = true;
            buttons[UP].color = button_disabled_color;
        }
        if (snake_entry_cell.y == GRID_DIM.ROWS-1) {
            buttons[DOWN].disabled = true;
            buttons[DOWN].color = button_disabled_color;
        }

        if (buttons[active_direction].disabled) {
            active_direction = !buttons[UP].disabled ? UP
                            : !buttons[DOWN].disabled ? DOWN
                            : !buttons[RIGHT].disabled ? RIGHT
                            : !buttons[LEFT].disabled ? LEFT
                            : NONE; // should not be possible
        }

        // -- DRAW --
        BeginDrawing();
        ClearBackground(BLACK);

        for (int i = 0; i < text_input_count; i++) {
            TextInput* text_input = &text_inputs[i];

            Font input_font = GetFontDefault();
            int input_font_size = 20;
            int input_spacing = 2;
            Vector2 text_size = MeasureTextEx(input_font, text_input->input, input_font_size, input_spacing);
            Vector2 title_size = MeasureTextEx(input_font, text_input->title, input_font_size, input_spacing);

            DrawRectangle(
                text_input->position.x,
                text_input->position.y,
                text_input->size.x,
                text_input->size.y,
                text_input->color
            );
            DrawTextEx(
                input_font,
                text_input->input,
                (Vector2) {
                    .x = text_input->position.x + text_input->size.x/2 - text_size.x/2,
                    .y = text_input->position.y + text_input->size.y/2 - text_size.y/2,
                },
                input_font_size,
                input_spacing,
                RED
            );
            DrawTextEx(
                input_font,
                text_input->title,
                (Vector2) {
                    .x = text_input->position.x,
                    .y = text_input->position.y - title_size.y - 10,
                },
                input_font_size,
                input_spacing,
                WHITE
            );
        }

        for (int i = 0; i < button_count; i++) {
            Button* button = &buttons[i];

            Font title_font = GetFontDefault();
            int title_font_size = 30;
            int title_spacing = 2;
            Vector2 text_size = MeasureTextEx(title_font, button->title, title_font_size, title_spacing);

            DrawRectangle(
                button->position.x,
                button->position.y,
                button->size.x,
                button->size.y,
                button->color
            );
            DrawTextEx(
                title_font,
                button->title,
                (Vector2) {
                    .x = button->position.x + button->size.x/2 - text_size.x/2,
                    .y = button->position.y + button->size.y/2 - text_size.y/2,
                },
                title_font_size,
                title_spacing,
                BLUE
            );
        }

        BeginMode2D(camera);
            grid_draw_system();
            grid_walls_draw_system(last_hovered_wall);

            Rectangle snake_head_content = {
                .x = snake_entry_cell.x * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
                .y = snake_entry_cell.y * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
                .width = GRID.CONTENT_CELL_SIZE,
                .height = GRID.CONTENT_CELL_SIZE,
            };
            Color snake_head_color = GREEN;
            DrawRectangleRec(snake_head_content, snake_head_color);

            Cell head_move_cell = cell_add_wrapping(snake_entry_cell, get_movement_direction(active_direction));
            Vector2 cell_mid = {
                .x = head_move_cell.x * GRID.CELL_SIZE + GRID.CELL_SIZE/2,
                .y = head_move_cell.y * GRID.CELL_SIZE + GRID.CELL_SIZE/2,
            };
            Vector2 c1 = {
                .x = head_move_cell.x * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
                .y = head_move_cell.y * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
            };
            Vector2 c2 = {
                .x = c1.x + GRID.CONTENT_CELL_SIZE,
                .y = c1.y,
            };
            Vector2 c3 = {
                .x = c1.x,
                .y = c1.y + GRID.CONTENT_CELL_SIZE,
            };
            Vector2 c4 = {
                .x = c1.x + GRID.CONTENT_CELL_SIZE,
                .y = c1.y + GRID.CONTENT_CELL_SIZE,
            };
            
            Vector2 t1, t2, t3;
            switch (active_direction) {
            case UP:
                t1 = c3;
                t2 = c4;
                t3 = (Vector2) {
                    .x = cell_mid.x,
                    .y = c1.y,
                };
                break;
            case DOWN:
                t1 = c2;
                t2 = c1;
                t3 = (Vector2) {
                    .x = cell_mid.x,
                    .y = c3.y,
                };
                break;
            case RIGHT:
                t1 = c1;
                t2 = c3;
                t3 = (Vector2) {
                    .x = c2.x,
                    .y = cell_mid.y,
                };
                break;
            case LEFT:
                t1 = c4;
                t2 = c2;
                t3 = (Vector2) {
                    .x = c1.x,
                    .y = cell_mid.y,
                };
                break;
            }

            DrawTriangle(t1, t2, t3, MAGENTA);

        EndMode2D();

        EndDrawing();
    }
}

void play_level(const GameLevel const* level) {
    GAME_STATE = PLAYING;

    init_grid_dimentions(level->rows, level->cols);
    init_fireworks();

    for (int i = 0; i < level->rows; i++) {
        for (int j = 0; j < level->cols + 1; j++) {
            GRID_WALLS.vertical[i][j] = level->vertical_walls[i][j];
        }
    }
    for (int i = 0; i < level->cols; i++) {
        for (int j = 0; j < level->rows + 1; j++) {
            GRID_WALLS.horizontal[i][j] = level->horizontal_walls[i][j];
        }
    }

    for (int i = 0; i < level->rows; i++) {
        for (int j = 0; j < level->cols + 1; j++) {
            GRID_CELL_OBJECTS.cell_position[i][j] = (Vector2) {
                .x = j * GRID.CELL_SIZE,
                .y = i * GRID.CELL_SIZE,
            };
        }
    }

    Image snake_head_sprite_image = LoadImage("assets\\snake-head-sprite.png");
    TextureHandle snake_head_sprite_image_handle = texture_assets_reserve_texture_slot(&TEXTURE_ASSETS);
    texture_assets_put_image_and_create_texture(&TEXTURE_ASSETS, snake_head_sprite_image_handle, snake_head_sprite_image);

    Image blood_image = LoadImage("assets\\blood-splatter.png");
    TextureHandle blood_image_handle = texture_assets_reserve_texture_slot(&TEXTURE_ASSETS);
    texture_assets_put_image_and_create_texture(&TEXTURE_ASSETS, blood_image_handle, blood_image);

    int explosion_texture_load_completed_event = 0;
    int explosion_textures_handle_count = 0;
    TextureHandle* explosion_textures_handles;
    AsyncioLoadTextureDir task_arg = {
        .texture_assets = &TEXTURE_ASSETS,
        .dirpath = "assets\\explosion",
        .completed_event = &explosion_texture_load_completed_event,
        .handle_count = &explosion_textures_handle_count,
        .handles = &explosion_textures_handles,
    };
    Task task = get_task_asyncio_load_texture_dir(&task_arg);
    thread_pool_add_task(THREAD_POOL, task);

    Music battle_audio = LoadMusicStream("assets\\battle.mp3");
    Music chill_town_audio = LoadMusicStream("assets\\chill-town.mp3");
    Music scream_audio = LoadMusicStream("assets\\scream3.mp3");
    Music explosion_audio = LoadMusicStream("assets\\explosion.mp3");

    PlayMusicStream(battle_audio);
    scream_audio.looping = false;
    explosion_audio.looping = false;

    SpriteAnimation explosion_animation = {0};
    bool explosion_animation_loaded = false;
    
    float UPDATE_RATE_PER_SEC = 2;

    Camera2D camera = {0};
    camera.zoom = 1.0;
    camera.offset = (Vector2) {
        .x = (SCREEN_WIDTH - GRID_DIM.WIDTH)/2.0,
        .y = (SCREEN_HEIGHT - GRID_DIM.HEIGHT)/2.0,
    };

    Snake snake = new_snake(level->snake_entry_cell, level->snake_entry_momentum);
    Cell food_cell = spawn_food(&snake);
    int food_eaten = 0;

    Vector2 food_origin = {
        .x = food_cell.x*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2,
        .y = food_cell.y*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2,
    };

    int frame = 0;
    float time_elapsed = 0.0;
    Movement buffered_movement = NONE;
    int food_eaten_prev_update = 0;
    int head_gameover_cycle = 0;
    int* last_hovered_wall = NULL;

    bool play_battle_audio = true;
    bool play_chill_town_audio = false;
    
    bool gameover_do_initiate_eaten_tail_event = false;

    bool gameover_do_initiate_explosion_event = false;
    bool gameover_explosion_initiated = false;
    bool gameover_explosion_start_event = false;
    bool gameover_explosion_end_event = false;

    bool explosion_scream_start_event = false;
    bool explosion_scream_end_event = false;
    
    bool level_should_exit = false;
    while (!level_should_exit) {
        float delta_time = GetFrameTime();
        time_elapsed += delta_time;

        if (gameover_explosion_initiated) {
            grid_cell_objects_position_update_system(delta_time);
        }

        if (explosion_texture_load_completed_event) {
            explosion_texture_load_completed_event = 0;

            for (int i = 0; i < explosion_textures_handle_count; i++) {
                texture_assets_create_texture_uncheched(&TEXTURE_ASSETS, explosion_textures_handles[i]);
            }

            float* animation_checkpoints = malloc(explosion_textures_handle_count * sizeof(float));
            for (int i = 0; i < explosion_textures_handle_count; i++) {
                animation_checkpoints[i] = 0.03 * (i+1);
            }

            SequenceTimer explosion_animation_timer = new_sequence_timer(
                animation_checkpoints, explosion_textures_handle_count, Timer_NonRepeating
            );
            explosion_animation = new_sprite_animation(
                explosion_animation_timer,
                explosion_textures_handles,
                explosion_textures_handle_count
            );

            explosion_animation_loaded = true;
        }

        if (gameover_do_initiate_eaten_tail_event) {
            gameover_do_initiate_eaten_tail_event = false;

            StopMusicStream(battle_audio);
            play_battle_audio = false;
            PlayMusicStream(chill_town_audio);
            play_chill_town_audio = true;
        }

        if (gameover_do_initiate_explosion_event) {
            gameover_do_initiate_explosion_event = false;

            Vector2 snake_head = {
                .x = snake.head.x * GRID.CELL_SIZE,
                .y = snake.head.y * GRID.CELL_SIZE,
            };
            float explosion_range = __min(GRID_DIM.ROWS, GRID_DIM.COLS) * GRID.CELL_SIZE;
            float max_init_speed = 150.0;

            for (int i = 0; i < level->rows; i++) {
                for (int j = 0; j < level->cols + 1; j++) {
                    Vector2 pos = GRID_CELL_OBJECTS.cell_position[i][j];
                    Vector2 dv = Vector2Subtract(pos, snake_head);
                    if (j == snake.head.x && i == snake.head.y) {
                        dv = (Vector2) {1, 0};
                    }

                    float dist = Vector2Length(dv);
                    float force = __max(explosion_range - dist, 0.0) / explosion_range;
                    float speed = force * max_init_speed;
                    
                    Vector2 vel = Vector2Scale(Vector2Normalize(dv), speed);
                    float rotate = (float)rand_in_range(0, 360);
                    Vector2 rand_vel = Vector2Rotate((Vector2) {1, 0}, rotate);
                    rand_vel = Vector2Scale(rand_vel, speed/3);

                    GRID_CELL_OBJECTS.cell_velocity[i][j] = Vector2Add(vel, rand_vel);
                }
            }

            gameover_explosion_initiated = true;
        }

        TextureHandle explosion_frame = get_current_texture(&explosion_animation);
        if (GAME_STATE == GAMEOVER_LOSE && explosion_animation_loaded) {
            tick_animation_timer(&explosion_animation, delta_time);
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            level_should_exit = true;
        }

        // -- BUFFER -----
        if (IsKeyPressed(KEY_UP) || IsKeyPressedRepeat(KEY_UP)) {
            buffered_movement = UP;
        }
        else if (IsKeyPressed(KEY_DOWN) || IsKeyPressedRepeat(KEY_DOWN)) {
            buffered_movement = DOWN;
        }
        else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressedRepeat(KEY_RIGHT)) {
            buffered_movement = RIGHT;
        }
        else if (IsKeyPressed(KEY_LEFT) || IsKeyPressedRepeat(KEY_LEFT)) {
            buffered_movement = LEFT;
        }

        if (IsKeyPressed('1') || IsKeyPressedRepeat('1')) {
            UPDATE_RATE_PER_SEC -= 0.5;
        }
        if (IsKeyPressed('2') || IsKeyPressedRepeat('2')) {
            UPDATE_RATE_PER_SEC += 0.5;
        }
        UPDATE_RATE_PER_SEC = Clamp(UPDATE_RATE_PER_SEC, 1.0, 6.0);

        // DEBUG: Mock Win Case
        mock_win_by_space_press_system();
        check_fireworks_started_system(&FIREWORKS_WINSCREEN);
        fireworks_update_system(delta_time, &FIREWORKS_WINSCREEN);

        // -- UPDATE -----
        if (time_elapsed >= 1.0/UPDATE_RATE_PER_SEC) {
            time_elapsed = 0.0;
            frame++;

            if (is_game_over()) {
                head_gameover_cycle++;
                head_gameover_cycle %= 2;
                goto POST_UPDATE;
            }

            CellVector move = {0};
            if (buffered_movement == NONE
                || snake.momentum == buffered_movement
                || snake.momentum == movement_negate(buffered_movement)
            ) {
                move = get_movement_direction(snake.momentum);
            }
            else {
                move = get_movement_direction(buffered_movement);
                snake.momentum = buffered_movement;
            }
            buffered_movement = NONE;

            bool move_success = move_snake(&snake, move);
            if (food_eaten_prev_update) {
                eat_food(&snake);
            }
            if (snake_len(&snake) == total_cell_count()) {
                GAME_STATE = GAMEOVER_WIN;
                goto POST_UPDATE;
            }
            if (!move_success) {
                GAME_STATE = GAMEOVER_LOSE;
                gameover_do_initiate_explosion_event = true;
                goto POST_UPDATE;
            }
            if (does_intersect(&snake)) {
                GAME_STATE = GAMEOVER_LOSE;
                gameover_do_initiate_eaten_tail_event = true;
                goto POST_UPDATE;
            }

            if (cell_equals(snake.head, food_cell)) {
                food_eaten_prev_update = 1;
                food_eaten++;
                food_cell = spawn_food(&snake);
                food_origin.x = food_cell.x*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2;
                food_origin.y = food_cell.y*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2;
            }
            else {
                food_eaten_prev_update = 0;
            }
        }
        POST_UPDATE:

        // -- AUDIO -----
        if (play_battle_audio) {
            UpdateMusicStream(battle_audio);
        }
        if (play_chill_town_audio) {
            UpdateMusicStream(chill_town_audio);
        }

        if (gameover_explosion_initiated && !gameover_explosion_start_event && !gameover_explosion_end_event) {
            StopMusicStream(battle_audio);
            play_battle_audio = false;

            PlayMusicStream(explosion_audio);
            gameover_explosion_start_event = true;
        }
        if (gameover_explosion_start_event && !gameover_explosion_end_event) {
            UpdateMusicStream(explosion_audio);
            if (!IsMusicStreamPlaying(explosion_audio)) {
                StopMusicStream(explosion_audio);
                gameover_explosion_end_event = true;
            }
        }

        if (gameover_explosion_end_event && !explosion_scream_start_event && !explosion_scream_end_event) {
            PlayMusicStream(chill_town_audio);
            play_chill_town_audio = true;

            PlayMusicStream(scream_audio);
            // SetMusicVolume(scream_audio, 1);
            explosion_scream_start_event = true;
        }
        if (explosion_scream_start_event && !explosion_scream_end_event) {
            UpdateMusicStream(scream_audio);
            if (!IsMusicStreamPlaying(scream_audio)) {
                StopMusicStream(scream_audio);
                explosion_scream_end_event = true;
            }
        }

        // -- DRAW -----
        BeginDrawing();

            ClearBackground(BLACK);

            BeginMode2D(camera);

                if (gameover_explosion_initiated) {
                    grid_cell_objects_draw_system();
                }
                else {
                    grid_draw_system();
                    grid_walls_draw_system(last_hovered_wall);
                }

                if (GAME_STATE == GAMEOVER_LOSE && gameover_explosion_initiated) {
                    Texture* texture_blood = texture_assets_get_texture_or_default(&TEXTURE_ASSETS, blood_image_handle);
                    float blood_size_y = 8 * GRID.CELL_SIZE;
                    float blood_size_x = blood_size_y * ((float)texture_blood->width / (float)texture_blood->height);
                    // printf("w h: %d %d | w/h: %0.2f | x y: %0.2f %0.2f | x/y: %0.2f\n",
                    //         texture_blood->width, texture_blood->height, (float)texture_blood->width / (float)texture_blood->height,
                    //         blood_size_x, blood_size_y, blood_size_x/blood_size_y);
                    DrawTexturePro(
                        *texture_blood,
                        (Rectangle) {
                            .x = 0,
                            .y = 0,
                            .width = texture_blood->width,
                            .height = texture_blood->height
                        },
                        (Rectangle) {
                            .x = (snake.head.x * GRID.CELL_SIZE)
                                + GRID.CELL_SIZE/2
                                - blood_size_x/2,
                            .y = (snake.head.y * GRID.CELL_SIZE)
                                + GRID.CELL_SIZE/2
                                - blood_size_y/2,
                            .width = blood_size_x,
                            .height = blood_size_y,
                        },
                        (Vector2) {0, 0},
                        0,
                        WHITE
                    );
                }

                snake_draw_system(&snake, head_gameover_cycle);
                
                if (GAME_STATE == GAMEOVER_LOSE) {
                    Texture* snake_head_sprite_texture = &TEXTURE_ASSETS.textures[snake_head_sprite_image_handle.id];
                    float rotation;
                    switch (snake.momentum) {
                    case UP:
                        rotation = 180;
                        break;
                    case DOWN:
                        rotation = 0;
                        break;
                    case RIGHT:
                        rotation = 90;
                        break;
                    case LEFT:
                        rotation = 270;
                        break;
                    }
                    DrawTexturePro(
                        *snake_head_sprite_texture,
                        (Rectangle) {
                            .x = 0,
                            .y = 0,
                            .width = snake_head_sprite_texture->width,
                            .height = snake_head_sprite_texture->height
                        },
                        (Rectangle) {
                            .x = snake.head.x * GRID.CELL_SIZE + GRID.CONTENT_CELL_SIZE/2 + GRID.CONTENT_MARGIN,
                            .y = snake.head.y * GRID.CELL_SIZE + GRID.CONTENT_CELL_SIZE/2 + GRID.CONTENT_MARGIN,
                            .width = GRID.CONTENT_CELL_SIZE,
                            .height = GRID.CONTENT_CELL_SIZE,
                        },
                        (Vector2) {GRID.CONTENT_CELL_SIZE/2, GRID.CONTENT_CELL_SIZE/2},
                        rotation,
                        WHITE
                    );
                }

                if (GAME_STATE != GAMEOVER_WIN && !gameover_explosion_initiated) {
                    DrawCircleV(food_origin, GRID.CONTENT_CELL_SIZE/2, YELLOW);
                }

                fireworks_draw_system(&FIREWORKS_WINSCREEN);

                if (GAME_STATE == GAMEOVER_LOSE && gameover_explosion_initiated) {
                    bool finished = sequence_timer_is_finished(&explosion_animation.timer);
                    if (explosion_animation_loaded && !finished) {
                        Texture* texture_explosion_frame = texture_assets_get_texture_or_default(&TEXTURE_ASSETS, explosion_frame);
                        float explosion_size = __max(GRID_DIM.ROWS, GRID_DIM.COLS) * GRID.CELL_SIZE;
                        DrawTexturePro(
                            *texture_explosion_frame,
                            (Rectangle) {
                                .x = 0,
                                .y = 0,
                                .width = texture_explosion_frame->width,
                                .height = texture_explosion_frame->height
                            },
                            (Rectangle) {
                                .x = snake.head.x * GRID.CELL_SIZE
                                    + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2
                                    - explosion_size/2,
                                .y = snake.head.y * GRID.CELL_SIZE
                                    + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2
                                    - explosion_size/2,
                                .width = explosion_size,
                                .height = explosion_size,
                            },
                            (Vector2) {0, 0},
                            0,
                            WHITE
                        );
                    }
                }
                
            EndMode2D();

            const char* frame_str = TextFormat("%d", frame);
            DrawText(frame_str, 15, 15, 20, RED);
            const char* food_eaten_str = TextFormat("Food: %d", food_eaten);
            DrawText(food_eaten_str, 15+50, 15, 20, RED);
            const char* speed_str = TextFormat("Speed: %0.1f", UPDATE_RATE_PER_SEC);
            DrawText(speed_str, 15+50, 15+50, 20, RED);

            if (GAME_STATE == GAMEOVER_WIN) {
                Font font_default = GetFontDefault();
                const char* text = "! ! !  YOU WIN  ! ! !";
                float font_size = 100;
                float text_spacing = 10;
                Vector2 text_measure = MeasureTextEx(font_default, text, font_size, text_spacing);
                Vector2 text_position = {
                    .x = SCREEN_WIDTH/2 - text_measure.x/2,
                    .y = SCREEN_HEIGHT/4 - text_measure.y/2,
                };
                Color text_color = GREEN;
                DrawTextEx(font_default, text, text_position, font_size, text_spacing, text_color);
            }

            if (GAME_STATE == GAMEOVER_LOSE) {
                Font font_default = GetFontDefault();
                const char* text = "GAME OVER";
                float font_size = 100;
                float text_spacing = 2;
                Vector2 text_measure = MeasureTextEx(font_default, text, font_size, text_spacing);
                Vector2 text_position = {
                    .x = SCREEN_WIDTH/2 - text_measure.x/2,
                    .y = SCREEN_HEIGHT/4 - text_measure.y/2,
                };
                Color text_color = RED;
                DrawTextEx(font_default, text, text_position, font_size, text_spacing, text_color);
            }

        EndDrawing();
    }

    StopMusicStream(battle_audio);
    StopMusicStream(chill_town_audio);
    StopMusicStream(scream_audio);
    StopMusicStream(explosion_audio);

    UnloadMusicStream(battle_audio);
    UnloadMusicStream(chill_town_audio);
    UnloadMusicStream(scream_audio);
    UnloadMusicStream(explosion_audio);

    for (int i = 0; i < explosion_textures_handle_count; i++) {
        if (TEXTURE_ASSETS.texture_ready[explosion_textures_handles[i].id]) {
            texture_assets_unload_texture(&TEXTURE_ASSETS, explosion_textures_handles[i]);
        }
    }
    if (explosion_textures_handle_count > 0) {
        free(explosion_textures_handles);
    }
}

double time_elapsed_ns(struct timespec tstart, struct timespec tend) {
    return ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - 
           ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
}

int main() {
    srand(time(NULL));

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake Game");
    InitAudioDevice();

    SetTargetFPS(30);

    THREAD_POOL = thread_pool_create(10, 100);
    TEXTURE_ASSETS = new_texture_assets();

    {
        const int CELL_SIZE = 50;
        const int GRID_LINE_THICKNESS = 4;
        const int CONTENT_MARGIN = GRID_LINE_THICKNESS + GRID_LINE_THICKNESS/2;
        const int CONTENT_CELL_SIZE = CELL_SIZE - 2*CONTENT_MARGIN;
        const int WALL_THICKNESS = 10;
        GRID = (Grid) {
            .CELL_SIZE = CELL_SIZE,
            .LINE_THICKNESS = GRID_LINE_THICKNESS,
            .CONTENT_MARGIN = CONTENT_MARGIN,
            .CONTENT_CELL_SIZE = CONTENT_CELL_SIZE,
            .WALL_THICKNESS = WALL_THICKNESS,
        };
    }

    int button_count = 3;
    Vector2 button_size = { 300, 100 };
    float button_margin_y = button_size.y/4;
    Vector2 first_button_position = {
        .x = SCREEN_WIDTH/2 - button_size.x/2,
        .y = SCREEN_HEIGHT/2 - (button_count * button_size.y + (button_count-1) * button_margin_y)/2,
    };
    Color button_idle_color = WHITE;
    Color button_hover_color = YELLOW;

    Button buttons[3] = {
        (Button) {
            .title = "PLAY",
        },
        (Button) {
            .title = "LEVEL EDIT",
        },
        (Button) {
            .title = "EXIT",
        },
    };

    for (int i = 0; i < button_count; i++) {
        Button* button = &buttons[i];
        button->id = i;
        button->position = first_button_position;
        button->position.y += i * (button_size.y + button_margin_y);
        button->size = button_size;
        button->color = button_idle_color;
        button->disabled = false;
    }

    GameLevel level_0 = load_level(0);

    TextureHandle texture_handle = texture_assets_reserve_texture_slot(&TEXTURE_ASSETS);
    Image snake_head_image = LoadImage("assets\\snake-head.png");
    texture_assets_put_image_and_create_texture(&TEXTURE_ASSETS, texture_handle, snake_head_image);

    float texture_size = button_size.x;
    Rectangle texture_src_rect = {
        .x = (snake_head_image.width - 530)/2,
        .y = (snake_head_image.height - 530)/2,
        .width = 530,
        .height = 530,
    };
    Rectangle texture_dist_rect_1 = {
        .x = buttons[1].position.x - 3*button_margin_y - texture_size,
        .y = buttons[1].position.y + buttons[1].size.y/2 - texture_size/2,
        .width = texture_size,
        .height = texture_size,
    };
    Rectangle texture_dist_rect_2 = {
        .x = buttons[1].position.x + buttons[1].size.x + 3*button_margin_y,
        .y = texture_dist_rect_1.y,
        .width = texture_dist_rect_1.width,
        .height = texture_dist_rect_1.height,
    };

    Music main_menu_audio = LoadMusicStream("assets\\main-menu.wav");
    PlayMusicStream(main_menu_audio);

    bool window_should_close = false;
    while (!window_should_close && !WindowShouldClose()) {
        float delta_time = GetFrameTime();
        
        UpdateMusicStream(main_menu_audio);

        int hovered_button_id = -1;
        for (int i = 0; i < button_count; i++) {
            if (Button_hovered(&buttons[i])) {
                hovered_button_id = i;
                buttons[i].color = button_hover_color;
            }
            else {
                buttons[i].color = button_idle_color;
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            play_level(&level_0);
        }

        if (hovered_button_id != -1 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            switch (hovered_button_id) {
            case 0: // PLAY
                PauseMusicStream(main_menu_audio);
                play_level(&level_0);
                ResumeMusicStream(main_menu_audio);
                break;
            case 1: // LEVEL EDIT
                edit_level(&level_0);
                save_level(0, &level_0);
                break;
            case 2: // EXIT
                window_should_close = true;
                break;
            }
        }

        // -- DRAW --
        BeginDrawing();
        ClearBackground(BLACK);

        Texture* texture_res = &TEXTURE_ASSETS.textures[texture_handle.id];
        DrawTexturePro(
            *texture_res,
            texture_src_rect,
            texture_dist_rect_1,
            (Vector2) {0, 0},
            0,
            WHITE
        );
        DrawTexturePro(
            *texture_res,
            texture_src_rect,
            texture_dist_rect_2,
            (Vector2) {0, 0},
            0,
            WHITE
        );

        for (int i = 0; i < button_count; i++) {
            Button* button = &buttons[i];

            Font title_font = GetFontDefault();
            int title_font_size = 30;
            int title_spacing = 2;
            Vector2 text_size = MeasureTextEx(title_font, button->title, title_font_size, title_spacing);

            DrawRectangle(
                button->position.x,
                button->position.y,
                button->size.x,
                button->size.y,
                button->color
            );
            DrawTextEx(
                title_font,
                button->title,
                (Vector2) {
                    .x = button->position.x + button->size.x/2 - text_size.x/2,
                    .y = button->position.y + button->size.y/2 - text_size.y/2,
                },
                title_font_size,
                title_spacing,
                BLUE
            );
        }

        EndDrawing();
    }

    StopMusicStream(main_menu_audio);
    UnloadMusicStream(main_menu_audio);

    thread_pool_shutdown(THREAD_POOL, THREADPOOL_GRACEFULL_SHUTDOWN);
    
    CloseWindow();
    
    return 0;
}