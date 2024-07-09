#ifndef SNAKEGAME_FIREWORKS
#define SNAKEGAME_FIREWORK

#include "stdbool.h"

#include "raylib.h"
#include "raymath.h"

#include "util.h"

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

void despawn_fireworks(Fireworks* fireworks);
void spawn_fireworks(Fireworks* fireworks);
void update_particle(FireworkParticle* particle, float lifetime_ratio, float delta_time);
void update_particles(Fireworks* fireworks, float delta_time);

typedef struct {
    bool fireworks_started;
    Fireworks* fireworks;
    int fireworks_count;
    int fireworks_index;
    SequenceTimer fireworks_timer;
} FireworksResources;

void init_fireworks();
void check_fireworks_started_system(FireworksResources* fireworks_resources);
void fireworks_update_system(float delta_time, FireworksResources* fireworks_resources);
void fireworks_draw_system(FireworksResources* fireworks_resources);

#endif