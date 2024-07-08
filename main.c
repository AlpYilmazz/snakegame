#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "time.h"
#include "string.h"

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"

#include "util.c"

// -- GLOBAL VARIABLE TYPES -----
typedef struct {
    int CELL_SIZE;
    int COLS;
    int ROWS;
    int LINE_THICKNESS;
    int CONTENT_MARGIN;
    int CONTENT_CELL_SIZE;
    int WALL_THICKNESS;
} Grid;

typedef struct {
    int vertical[20][20];   // [row_i][col_i], [row_i][col_i + 1] -> [0][0], [0][1]
    int horizontal[20][20]; // [col_i][row_i], [col_i][row_i + 1]
} GridWalls;

typedef enum {
    // INIT, PAUSE,
    PLAYING,
    GAMEOVER_LOSE,
    GAMEOVER_WIN,
} GameState;
// ---------------------

// -- GLOBAL VARIABLES -----
const int SCREEN_WIDTH = 1000;
const int SCREEN_HEIGHT = 1000;

Grid GRID;
GridWalls GRID_WALLS;
int GRID_GROUND_COUNT;
GameState GAME_STATE;

int is_game_over() {
    return GAME_STATE == GAMEOVER_LOSE || GAME_STATE == GAMEOVER_WIN;
}
// -------------------------

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

Movement movement_negate(Movement move) {
    switch (move) {
        case UP:
            return DOWN;
        case DOWN:
            return UP;
        case RIGHT:
            return LEFT;
        case LEFT:
            return RIGHT;
        case NONE:
            return NONE;
    }
}

CellVector get_movement_direction(Movement move) {
    switch (move) {
        case UP:
            return (CellVector) { .x = 0.0, .y = -1.0 };
        case DOWN:
            return (CellVector) { .x = 0.0, .y = 1.0 };
        case RIGHT:
            return (CellVector) { .x = 1.0, .y = 0.0 };
        case LEFT:
            return (CellVector) { .x = -1.0, .y = 0.0 };
        case NONE:
            return (CellVector) { .x = 0.0, .y = 0.0 };
    }
}

int cell_equals(Cell this, Cell other) {
    return (this.x == other.x) && (this.y == other.y);
}

Cell cell_add_wrapping(Cell cell, CellVector add) {
    return (Cell) {
        .x = (cell.x + GRID.COLS + add.x)%GRID.COLS,
        .y = (cell.y + GRID.ROWS + add.y)%GRID.ROWS,
    };
}

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

    if (GRID_WALLS.vertical[row][wall_v] || GRID_WALLS.horizontal[col][wall_h]) {
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
            .x = rand() % GRID.COLS,
            .y = rand() % GRID.ROWS,
        };
    } while(occupies_cell(snake, food_cell));
    return food_cell;
}

typedef struct {
    Vector2 position;
    Vector2 velocity;
    float rotation;
    float spawn_scale;
    float scale;
} FireworkParticle;

typedef struct {
    Vector2 origin;

    float spawn_radius_min;
    float spawn_radius_max;
    float start_angle;
    float end_angle;
    float angle_increment;
    float particles_per_angle; // > 1

    float particle_speed;
    float particle_ttl;
    float particle_spawn_size;

    Color particle_color;

    // TODO: Init array with size and do not free/alloc each time
    FireworkParticle* particles;
    int particle_count;
    float particle_lifetime;
} Fireworks;

void despawn_fireworks(Fireworks* fireworks) {
    if (fireworks->particle_count > 0) {
        free(fireworks->particles);
        fireworks->particle_count = 0;
    }
}

void spawn_fireworks(Fireworks* fireworks) {
        const Vector2 unit_x = { 1.0, 0.0 };
        const Vector3 unit_x_v3 = { 1.0, 0.0, 0.0 };
        const float TWO_PI = 2.0*PI;
        const float START_ANGLE = fireworks->start_angle;
        const float END_ANGLE = fireworks->end_angle;
        const float ANGLE_INCREMENT = fireworks->angle_increment;
        const float PARTICLES_PER_ANGLE = fireworks->particles_per_angle; // > 1
        // const float ANGLE_INCREMENT = PI/40.0;

        const float radius_gap = fireworks->spawn_radius_max - fireworks->spawn_radius_min;
        const float radius_increment = radius_gap / (PARTICLES_PER_ANGLE-1);

        // const int ANGLE_COUNT = (int)(TWO_PI/ANGLE_INCREMENT) + 1;
        const int ANGLE_COUNT = (int)((END_ANGLE - START_ANGLE)/ANGLE_INCREMENT) + 1;
        int particle_count = 0;
        int capacity = 10 + ANGLE_COUNT * PARTICLES_PER_ANGLE * sizeof(FireworkParticle);
        FireworkParticle* particles = malloc(capacity);

        float angle = START_ANGLE;
        for (int i = 0; i < ANGLE_COUNT; i++, angle += ANGLE_INCREMENT) {
            Vector2 direction = Vector2Rotate(unit_x, -angle);
            for (int j = 0; j < PARTICLES_PER_ANGLE; j++) {
                float radius_factor = j * radius_increment;
                float position_factor = fireworks->spawn_radius_min + radius_factor;
                // TODO: add randomness to position
                Vector2 position = Vector2Add(fireworks->origin, Vector2Scale(direction, position_factor));
                
                int speed_factor = (int)fireworks->particle_speed;
                float velocity_factor = (float)rand_in_range(speed_factor*2, speed_factor*4);
                Vector2 velocity = Vector2Scale(
                    Vector2Normalize(Vector2Subtract(position, fireworks->origin)),
                    velocity_factor + radius_factor
                );
                
                float rotation = (float)(rand()%360 + 1);

                particles[particle_count] = (FireworkParticle) {
                    .position = position,
                    .velocity = velocity,
                    .rotation = rotation,
                    .spawn_scale = fireworks->particle_spawn_size,
                    .scale = fireworks->particle_spawn_size,
                };
                particle_count++;
            }
        }

        fireworks->particles = particles;
        fireworks->particle_count = particle_count;
        fireworks->particle_lifetime = fireworks->particle_ttl;
}

void update_particle(FireworkParticle* particle, float lifetime_ratio, float delta_time) {
    const Vector2 GRAVITY = { .x = 0.0, .y = 900.0 };
    const float DRAG_COEFF = 1.0 / 10.0;
    const Vector2 neg_velocity_norm = Vector2Normalize(Vector2Negate(particle->velocity));
    const float v_len = Vector2Length(particle->velocity);
    const float v_sq = v_len; // * v_len;
    
    const Vector2 AIR_DRAG = Vector2Scale(
        neg_velocity_norm,
        DRAG_COEFF * (v_sq/2) * particle->scale
    );
    Vector2 accelaration = Vector2Add(AIR_DRAG, GRAVITY);

    particle->velocity = Vector2Add(
        particle->velocity,
        Vector2Scale(accelaration, delta_time)
    );
    particle->position = Vector2Add(
        particle->position,
        Vector2Scale(particle->velocity, delta_time)
    );
    particle->rotation += delta_time * 360.0;
    particle->scale = particle->spawn_scale * lifetime_ratio;
}

void update_particles(Fireworks* fireworks, float delta_time) {
    if (fireworks->particle_count == 0) {
        return;
    }

    fireworks->particle_lifetime -= delta_time;
    if (fireworks->particle_lifetime <= 0) {
        despawn_fireworks(fireworks);
        return;
    }
    
    float lifetime_ratio = fireworks->particle_lifetime / fireworks->particle_ttl;
    for (int p = 0; p < fireworks->particle_count; p++) {
        update_particle(&fireworks->particles[p], lifetime_ratio, delta_time);
    }
}

typedef struct {
    bool fireworks_started;
    Fireworks* fireworks;
    int fireworks_count;
    int fireworks_index;
    SequenceTimer fireworks_timer;
} FireworksResources;

FireworksResources FIREWORKS_WINSCREEN;

void init_fireworks() {
    Fireworks fireworks_1 = {
        // .origin = Vector2Zero(),
        // .origin = (Vector2) {
        //     .x = 5.0,
        //     .y = 5.0,
        // },
        .origin = (Vector2) {
            .x = (GRID.CELL_SIZE * GRID.COLS)/2.0,
            .y = (GRID.CELL_SIZE * (GRID.ROWS + 10))/2.0,
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
        .x = (GRID.CELL_SIZE * (GRID.COLS + 6))/2.0,
        .y = (GRID.CELL_SIZE * (GRID.ROWS + 16))/2.0,
    };
    fireworks_2.particle_speed = 700.0;
    fireworks_2.particle_spawn_size = 30.0;
    fireworks_2.particle_color = BLUE;

    Fireworks fireworks_3 = fireworks_1;
    fireworks_3.origin = (Vector2) {
        .x = (GRID.CELL_SIZE * (GRID.COLS - 6))/2.0,
        .y = (GRID.CELL_SIZE * (GRID.ROWS + 26))/2.0,
    };
    fireworks_3.particle_speed = 700.0;
    fireworks_3.particle_ttl = 4.0;
    fireworks_3.particle_color = MAGENTA;

    float sq[4] = { 0.5, 1.2, 1.5, 3.0 };
    SequenceTimer fireworks_timer = new_sequence_timer(&sq[0], 4);
    bool fireworks_started = false;
    int fireworks_index = 0;
    const int fireworks_count = 3;
    Fireworks fireworks_winscreen[3] = {
        fireworks_1,
        fireworks_2,
        fireworks_3,
    };

    FIREWORKS_WINSCREEN = (FireworksResources) {
        .fireworks_started = false,
        .fireworks = &fireworks_winscreen[0],
        .fireworks_count = fireworks_count,
        .fireworks_index = 0,
        .fireworks_timer = fireworks_timer,
    };
}

void check_fireworks_started_system(
    FireworksResources* fireworks_resources
) {
    if (IsKeyPressed(KEY_SPACE)) {
        fireworks_resources->fireworks_started = true;
    }
}

void fireworks_update_system(
    float delta_time,
    FireworksResources* fireworks_resources
) {
    bool fireworks_started = fireworks_resources->fireworks_started;
    Fireworks* fireworks_ref = fireworks_resources->fireworks;
    int fireworks_count = fireworks_resources->fireworks_count;
    int* fireworks_index_ref = &fireworks_resources->fireworks_index;
    SequenceTimer* fireworks_timer_ref = &fireworks_resources->fireworks_timer;

    for (int i = 0; i < fireworks_count; i++) {
        update_particles(&fireworks_ref[i], delta_time);
    }

    if (fireworks_started) {
        tick_sequence_timer(fireworks_timer_ref, delta_time);
        // printf("%f -> %d -> %d\n", fireworks_timer.time_elapsed, fireworks_timer.index, fireworks_timer.pulsed);
        if (sequence_timer_has_pulsed(fireworks_timer_ref)) {
            Fireworks* fireworks_i = &fireworks_ref[*fireworks_index_ref];
            despawn_fireworks(fireworks_i);
            spawn_fireworks(fireworks_i);

            *fireworks_index_ref = (*fireworks_index_ref + 1) % fireworks_count;
        }
    }
}

void fireworks_draw_system(
    FireworksResources* fireworks_resources
) {
    Fireworks* fireworks = fireworks_resources->fireworks;
    int fireworks_count = fireworks_resources->fireworks_count;
    for (int f_i = 0; f_i < fireworks_count; f_i++) {
        Fireworks* fireworks_i = &fireworks[f_i];
        for (int p_i = 0; p_i < fireworks_i->particle_count; p_i++) {
            FireworkParticle particle = fireworks_i->particles[p_i];
            // TODO: animate size, rotation and color alpha
            float particle_size = particle.scale;
            Color particle_color = fireworks_i->particle_color;

            Rectangle particle_rect = {
                .x = particle.position.x - particle_size/2,
                .y = particle.position.y - particle_size/2,
                .width = particle_size,
                .height = particle_size,
            };
            DrawRectanglePro(
                particle_rect,
                (Vector2){ particle_rect.width/2.0, particle_rect.height/2.0 },
                particle.rotation,
                particle_color
            );
        }
    }
}

typedef struct {
    int rows;
    int cols;
    Cell snake_entry_cell;
    Movement snake_entry_momentum;
    int vertical_walls[20][20];
    int horizontal_walls[20][20];
} GameLevel;

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

void mock_win_by_space_press_system() {
    if (IsKeyPressed(KEY_SPACE)) {
        GAME_STATE = GAMEOVER_WIN;
    }
}

typedef struct {
    int id;
    char input[21];
    Vector2 position;
    Vector2 size;
    Color color;
    bool disabled;
} TextInput;

void TextInput_input(TextInput* text_input, char input) {
    int len = strlen(text_input->input);
    if (len < 21 - 1) { // at least one '\0' slot
        text_input->input[len] = input;
        text_input->input[len+1] = '\0';
    }
}

void TextInput_delete_back(TextInput* text_input) {
    int len = strlen(text_input->input);
    if (len > 0) {
        text_input->input[len-1] = '\0';
    }
}

bool TextInput_hovered(TextInput* text_input) {
    Vector2 mouse = GetMousePosition();
    return text_input->position.x <= mouse.x && mouse.x <= text_input->position.x + text_input->size.x
        && text_input->position.y <= mouse.y && mouse.y <= text_input->position.y + text_input->size.y;
}

typedef struct {
    int id;
    char title[20];
    Vector2 position;
    Vector2 size;
    Color color;
    bool disabled;
} Button;

bool Button_hovered(Button* button) {
    Vector2 mouse = GetMousePosition();
    return button->position.x <= mouse.x && mouse.x <= button->position.x + button->size.x
        && button->position.y <= mouse.y && mouse.y <= button->position.y + button->size.y;
}

void edit_level(GameLevel const* level) {
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
        .input = {0},
        .position = buttons_div_position,
        .size = sq_small_button_size,
    };
    TextInput cols_text_input = (TextInput) {
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

    int active_text_input_id = -1;

    bool editor_should_close = false;
    while (!editor_should_close && !WindowShouldClose()) {
        int hovered_text_input_id = -1;
        for (int i = 0; i < text_input_count; i++) {
            if (TextInput_hovered(&text_inputs[i])) {
                hovered_text_input_id = i;
            }
        }

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

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (hovered_button_id != -1) {
                // buttons[hovered_button_id].color = button_idle_color;

                switch (hovered_button_id) {
                case 0: // UPDATE GRID
                    sscanf(&text_inputs[0].input[0], "%d", &level->rows);
                    sscanf(&text_inputs[1].input[0], "%d", &level->cols);
                    break;
                case 1: // UP
                    break;
                case 2: // DOWN
                    break;
                case 3: // LEFT
                    break;
                case 4: // RIGHT
                    break;
                case 5: // SAVE
                    editor_should_close = true;
                    break;
                }
            }
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

        // -- DRAW --
        BeginDrawing();
        ClearBackground(BLACK);

        for (int i = 0; i < text_input_count; i++) {
            TextInput* text_input = &text_inputs[i];

            Font input_font = GetFontDefault();
            int input_font_size = 20;
            int input_spacing = 2;
            Vector2 text_size = MeasureTextEx(input_font, text_input->input, input_font_size, input_spacing);

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

        EndDrawing();
    }
}

void play_level(const GameLevel const* level) {
    GAME_STATE = PLAYING;

    GRID.ROWS = level->rows;
    GRID.COLS = level->cols;
    GRID_GROUND_COUNT = GRID.ROWS * GRID.COLS;

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
    
    float UPDATE_RATE_PER_SEC = 2;

    Camera2D camera = {0};
    camera.zoom = 1.0;
    camera.offset = (Vector2) {
        .x = (SCREEN_WIDTH - GRID.CELL_SIZE * GRID.COLS)/2.0,
        .y = (SCREEN_HEIGHT - GRID.CELL_SIZE * GRID.ROWS)/2.0,
    };

    Rectangle grid_frame = {
        .x = 0.0,
        .y = 0.0,
        .width = GRID.CELL_SIZE * GRID.COLS,
        .height = GRID.CELL_SIZE * GRID.ROWS,
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
    
    bool level_should_exit = false;
    while (!level_should_exit) {
        float delta_time = GetFrameTime();
        time_elapsed += delta_time;

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

        // Vector2 mouse = GetMousePosition();
        // Vector2 mouse_world = GetScreenToWorld2D(mouse, camera);
        // mouse_world.x += GRID.CELL_SIZE;
        // mouse_world.y += GRID.CELL_SIZE;
        // int row = (int)mouse_world.y / GRID.CELL_SIZE;
        // int col = (int)mouse_world.x / GRID.CELL_SIZE;
        // int row_u = ((int)mouse_world.y - 10) / GRID.CELL_SIZE;
        // int row_d = ((int)mouse_world.y + 10) / GRID.CELL_SIZE;
        // int col_l = ((int)mouse_world.x - 10) / GRID.CELL_SIZE;
        // int col_r = ((int)mouse_world.x + 10) / GRID.CELL_SIZE;

        // if (0 <= row && row <= (GRID.ROWS+1) * GRID.CELL_SIZE
        //     && 0 <= col && col <= (GRID.COLS+1) * GRID.CELL_SIZE
        // ) {
        //     if (col < col_r) {
        //         // wall: vertical[row][col+1]
        //         last_hovered_wall = &GRID_WALLS.vertical[row-1][col-1+1];
        //     }
        //     else if (col_l < col) {
        //         // wall: vertical[row][col]
        //         last_hovered_wall = &GRID_WALLS.vertical[row-1][col-1];
        //     }
        //     else if (row < row_d) {
        //         // wall: horizontal[col][row+1]
        //         last_hovered_wall = &GRID_WALLS.horizontal[col-1][row-1+1];
        //     }
        //     else if (row_u < row) {
        //         // wall: horizontal[col][row]
        //         last_hovered_wall = &GRID_WALLS.horizontal[col-1][row-1];
        //     }
        //     else if (last_hovered_wall != NULL) {
        //         last_hovered_wall = NULL;
        //     }
        // }
        // else {
        //     last_hovered_wall = NULL;
        // }

        // if (last_hovered_wall != NULL && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        //     *last_hovered_wall = !*last_hovered_wall;
        // }

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
            if (snake_len(&snake) == GRID_GROUND_COUNT) {
                GAME_STATE = GAMEOVER_WIN;
                goto POST_UPDATE;
            }
            if (!move_success) {
                GAME_STATE = GAMEOVER_LOSE;
                goto POST_UPDATE;
            }
            if (does_intersect(&snake)) {
                GAME_STATE = GAMEOVER_LOSE;
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

        // -- DRAW -----
        BeginDrawing();

            ClearBackground(BLACK);

            BeginMode2D(camera);

                DrawRectangleLinesEx(grid_frame, GRID.LINE_THICKNESS, BLUE);
                DrawRectangleRec(grid_frame, BEIGE);

                for (int i = 0; i < GRID.ROWS; i++) {
                    for (int j = 0; j < GRID.COLS; j++) {
                        Rectangle cell = {
                            .x = j * GRID.CELL_SIZE,
                            .y = i * GRID.CELL_SIZE,
                            .width = GRID.CELL_SIZE,
                            .height = GRID.CELL_SIZE,
                        };
                        DrawRectangleLinesEx(cell, GRID.LINE_THICKNESS/2, DARKGRAY);
                    }
                }

                for (int i = 0; i < GRID.ROWS; i++) {
                    for (int w = 0; w < GRID.COLS+1; w++) {
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
                for (int i = 0; i < GRID.COLS; i++) {
                    for (int w = 0; w < GRID.ROWS+1; w++) {
                        int* wall = &GRID_WALLS.horizontal[i][w];
                        Rectangle wall_rect = {
                            .x = (i * GRID.CELL_SIZE),// + GRID.WALL_THICKNESS/2,
                            .y = (w * GRID.CELL_SIZE) - GRID.WALL_THICKNESS/2,
                            .width = GRID.CELL_SIZE,// - GRID.WALL_THICKNESS,
                            .height = GRID.WALL_THICKNESS,
                        };
                        Color wall_color = BLUE;
                        if (*wall) {
                            DrawRectangleRec(wall_rect, wall_color);
                        }
                        if (wall == last_hovered_wall) {
                            wall_color.a = 150;
                            DrawRectangleRec(wall_rect, wall_color);
                        }
                    }
                }

                for (int tail_i = 0; tail_i < snake.tail_len; tail_i++) {
                    Cell snake_tail = snake.tail[tail_i];
                    float tail_size = (float)GRID.CONTENT_CELL_SIZE * 0.7;
                    float tail_margin = (GRID.CONTENT_CELL_SIZE - tail_size)/2.0;
                    Rectangle snake_tail_content = {
                        .x = snake_tail.x * GRID.CELL_SIZE + tail_margin + GRID.CONTENT_MARGIN,
                        .y = snake_tail.y * GRID.CELL_SIZE + tail_margin + GRID.CONTENT_MARGIN,
                        .width = tail_size,
                        .height = tail_size,
                    };
                    DrawRectangleRec(snake_tail_content, snake.tail_color);
                }

                Rectangle snake_head_content = {
                    .x = snake.head.x * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
                    .y = snake.head.y * GRID.CELL_SIZE + GRID.CONTENT_MARGIN,
                    .width = GRID.CONTENT_CELL_SIZE,
                    .height = GRID.CONTENT_CELL_SIZE,
                };
                Color snake_head_color = (head_gameover_cycle == 0)
                                        ? snake.head_color
                                        : snake.head_color_gameover_cycle;
                DrawRectangleRec(snake_head_content, snake_head_color);

                if (GAME_STATE != GAMEOVER_WIN) {
                    DrawCircleV(food_origin, GRID.CONTENT_CELL_SIZE/2, YELLOW);
                }

                fireworks_draw_system(&FIREWORKS_WINSCREEN);
                
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
                    .y = SCREEN_HEIGHT/2 - text_measure.y/2,
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
                    .y = SCREEN_HEIGHT/2 - text_measure.y/2,
                };
                Color text_color = RED;
                DrawTextEx(font_default, text, text_position, font_size, text_spacing, text_color);
            }

        EndDrawing();
    }
}

int main() {
    srand(time(NULL));

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake Game");
    
    SetTargetFPS(30);

    {
        const int CELL_SIZE = 50;
        const int GRID_LINE_THICKNESS = 4;
        const int CONTENT_MARGIN = GRID_LINE_THICKNESS + GRID_LINE_THICKNESS/2;
        const int CONTENT_CELL_SIZE = CELL_SIZE - 2*CONTENT_MARGIN;
        const int WALL_THICKNESS = 10;
        GRID = (Grid) {
            .CELL_SIZE = CELL_SIZE,
            .COLS = 0,
            .ROWS = 0,
            .LINE_THICKNESS = GRID_LINE_THICKNESS,
            .CONTENT_MARGIN = CONTENT_MARGIN,
            .CONTENT_CELL_SIZE = CONTENT_CELL_SIZE,
            .WALL_THICKNESS = WALL_THICKNESS,
        };
        
        // GRID_WALLS = (GridWalls) {
        //     .vertical = {0},
        //     .horizontal = {0},
        // };

        // for (int i = 0; i < ROWS; i++) {
        //     GRID_WALLS.vertical[i][COLS/2] = 1;
        // }
        // for (int i = 0; i < COLS; i++) {
        //     GRID_WALLS.horizontal[i][ROWS/2] = 1;
        // }
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

    bool window_should_close = false;
    while (!window_should_close && !WindowShouldClose()) {
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

        if (hovered_button_id != -1 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            switch (hovered_button_id) {
            case 0: // PLAY
                play_level(&level_0);
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
    
    CloseWindow();
    
    return 0;
}