#include "stdlib.h"
#include "stdio.h"
#include "stdbool.h"
#include "time.h"

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
} Grid;

typedef enum {
    // INIT, PAUSE,
    PLAYING,
    GAMEOVER_LOSE,
    GAMEOVER_WIN,
} GameState;
// ---------------------

// -- GLOBAL VARIABLES -----
Grid GRID;
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

void move_snake(Snake* snake, CellVector move) {
    Cell follow = snake->head;
    snake->head = cell_add_wrapping(snake->head, move);
    for (int tail_i = 0; tail_i < snake->tail_len; tail_i++) {
        Cell follow_temp = snake->tail[tail_i];
        snake->tail[tail_i] = follow;
        follow = follow_temp;
    }
    snake->tail_drag = follow;
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

int main() {
    srand(time(NULL));

    float UPDATE_RATE_PER_SEC = 2;

    const int SCREEN_WIDTH = 1000;
    const int SCREEN_HEIGHT = 1000;

    {
        const int CELL_SIZE = 50;
        const int GRID_LINE_THICKNESS = 4;
        const int CONTENT_MARGIN = GRID_LINE_THICKNESS + GRID_LINE_THICKNESS/2;
        const int CONTENT_CELL_SIZE = CELL_SIZE - 2*CONTENT_MARGIN;
        GRID = (Grid) {
            .CELL_SIZE = 50,
            .COLS = 10,
            .ROWS = 15,
            .LINE_THICKNESS = GRID_LINE_THICKNESS,
            .CONTENT_MARGIN = CONTENT_MARGIN,
            .CONTENT_CELL_SIZE = CONTENT_CELL_SIZE,
        };
        GRID_GROUND_COUNT = GRID.ROWS * GRID.COLS;
        GAME_STATE = PLAYING;
    }
    
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Snake Game");
    
    SetTargetFPS(30);
    
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

    Fireworks fireworks_midscreen = {
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

    Fireworks fireworks_midscreen_2 = fireworks_midscreen;
    fireworks_midscreen_2.origin = (Vector2) {
        .x = (GRID.CELL_SIZE * (GRID.COLS + 6))/2.0,
        .y = (GRID.CELL_SIZE * (GRID.ROWS + 16))/2.0,
    };
    fireworks_midscreen_2.particle_speed = 700.0;
    fireworks_midscreen_2.particle_spawn_size = 30.0;
    fireworks_midscreen_2.particle_color = BLUE;

    Fireworks fireworks_midscreen_3 = fireworks_midscreen;
    fireworks_midscreen_3.origin = (Vector2) {
        .x = (GRID.CELL_SIZE * (GRID.COLS - 6))/2.0,
        .y = (GRID.CELL_SIZE * (GRID.ROWS + 26))/2.0,
    };
    fireworks_midscreen_3.particle_speed = 700.0;
    fireworks_midscreen_3.particle_ttl = 4.0;
    fireworks_midscreen_3.particle_color = MAGENTA;

    float sq[4] = { 0.5, 1.2, 1.5, 3.0 };
    SequenceTimer fireworks_timer = new_sequence_timer(&sq[0], 4);
    bool fireworks_started = false;
    int fireworks_index = 0;
    const int fireworks_count = 3;
    Fireworks fireworks_winscreen[3] = {
        fireworks_midscreen,
        fireworks_midscreen_2,
        fireworks_midscreen_3,
    };

    // printf(
    //     "%0.2f\n%d\n%d\n%0.2f, %0.2f, %0.2f, %0.2f\n",
    //     fireworks_timer.time_elapsed,
    //     fireworks_timer.checkpoint_count,
    //     fireworks_timer.index,
    //     fireworks_timer.checkpoints[0],
    //     fireworks_timer.checkpoints[1],
    //     fireworks_timer.checkpoints[2],
    //     fireworks_timer.checkpoints[3]
    // );

    Snake snake = new_snake((Cell) { .x = 0, .y = 1 }, RIGHT);
    Cell food_cell = spawn_food(&snake);
    int food_eaten = 0;

    Vector2 food = {
        .x = food_cell.x*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2,
        .y = food_cell.y*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2,
    };

    int frame = 0;
    float time_elapsed = 0.0;
    Movement buffered_movement = NONE;
    int food_eaten_prev_update = 0;
    int head_gameover_cycle = 0;

    while (!WindowShouldClose()) {
        float delta_time = GetFrameTime();
        time_elapsed += delta_time;

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
        if (IsKeyPressed(KEY_SPACE)) {
            GAME_STATE = GAMEOVER_WIN;

            fireworks_started = true;
        }

        for (int i = 0; i < fireworks_count; i++) {
            update_particles(&fireworks_winscreen[i], delta_time);
        }

        if (fireworks_started) {
            tick_sequence_timer(&fireworks_timer, delta_time);
            // printf("%f -> %d -> %d\n", fireworks_timer.time_elapsed, fireworks_timer.index, fireworks_timer.pulsed);
            if (sequence_timer_has_pulsed(&fireworks_timer)) {
                Fireworks* fireworks = &fireworks_winscreen[fireworks_index];
                despawn_fireworks(fireworks);
                spawn_fireworks(fireworks);

                fireworks_index++;
                fireworks_index %= fireworks_count;
            }
        }

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

            move_snake(&snake, move);
            if (food_eaten_prev_update) {
                eat_food(&snake);
            }
            if (snake_len(&snake) == GRID_GROUND_COUNT) {
                GAME_STATE = GAMEOVER_WIN;
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
                food.x = food_cell.x*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2;
                food.y = food_cell.y*GRID.CELL_SIZE + GRID.CONTENT_MARGIN + GRID.CONTENT_CELL_SIZE/2;
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
                        DrawRectangleLinesEx(cell, GRID.LINE_THICKNESS/2, BLUE);
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
                    DrawCircleV(food, GRID.CONTENT_CELL_SIZE/2, YELLOW);
                }

                for (int f_i = 0; f_i < fireworks_count; f_i++) {
                    Fireworks* fireworks = &fireworks_winscreen[f_i];
                    for (int p_i = 0; p_i < fireworks->particle_count; p_i++) {
                        FireworkParticle particle = fireworks->particles[p_i];
                        // TODO: animate size, rotation and color alpha
                        float particle_size = particle.scale;
                        Color particle_color = fireworks->particle_color;

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
    
    CloseWindow();
    
    return 0;
}