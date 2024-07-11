#include <stdlib.h>
#include <stdbool.h>

#include "raylib.h"
#include "raymath.h"

#include "util.h"
#include "grid.h"
#include "animation.h"

#include "fireworks.h"

extern Grid GRID;
extern GridDimentions GRID_DIM;

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