#ifndef SNAKEGAME_ANIMATION
#define SNAKEGAME_ANIMATION

#include "asset.h"

typedef enum {
    Timer_NonRepeating,
    Timer_Repeating,
} TimerMode;

typedef struct {
    TimerMode mode;
    float time_setup;
    float time_elapsed;
    bool finished;
} Timer;

Timer new_timer(float setup_secs, TimerMode mode);
void tick_timer(Timer* timer, float delta_time_seconds);
bool timer_is_finished(Timer* timer);

typedef struct {
    int checkpoint_count;
    int index;
    float* checkpoints;
    float time_elapsed;
    bool pulsed;
} SequenceTimer;

SequenceTimer new_sequence_timer(float* checkpoints, int count);
void tick_sequence_timer(SequenceTimer* stimer, float delta_time_seconds);
bool sequence_timer_has_pulsed(SequenceTimer* stimer);

// typedef struct {
//     bool success;
//     int count;
//     int capacity;
//     Texture* items;
// } TextureList;

// void TextureList_push_back(TextureList* list, Texture item);
// TextureList load_texture_directory(const char* dir);

typedef struct {
    SequenceTimer timer;
    TextureHandle* textures;
    int texture_count;
    int current_texture_ind;
} SpriteAnimation;

SpriteAnimation new_sprite_animation(SequenceTimer timer, TextureHandle* textures, int texture_count);
void tick_animation_timer(SpriteAnimation* anim, float delta_time_seconds);
TextureHandle get_current_texture(SpriteAnimation* anim);

#endif