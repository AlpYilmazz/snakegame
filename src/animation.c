// #include <windows.h>
#include <stdio.h>
#include <stdbool.h>

#include "asset.h"

#include "animation.h"

Timer new_timer(float setup_secs, TimerMode mode) {
    return (Timer) {
        .mode = mode,
        .time_setup = setup_secs,
        .time_elapsed = 0.0,
        .finished = false,
    };
}

void tick_timer(Timer* timer, float delta_time_seconds) {
    switch (timer->mode) {
    case Timer_NonRepeating:
        if (!timer->finished) {
            timer->time_elapsed += delta_time_seconds;
            if (timer->time_elapsed >= timer->time_setup) {
                timer->finished = true;
            }
        }
        break;
    case Timer_Repeating:
        timer->time_elapsed += delta_time_seconds;
        if (timer->time_elapsed >= timer->time_setup) {
            timer->time_elapsed = 0.0;
            timer->finished = true;
        }
        break;
    }
}

bool timer_is_finished(Timer* timer) {
    switch (timer->mode) {
    case Timer_NonRepeating:
        return timer->finished;
    case Timer_Repeating:
        if (timer->finished) {
            timer->finished = false;
            return true;
        }
        return false;
    }
}

SequenceTimer new_sequence_timer(float* checkpoints, int count, TimerMode mode) {
    return (SequenceTimer) {
        .mode = mode,
        .checkpoint_count = count,
        .index = 0,
        .checkpoints = checkpoints,
        .time_elapsed = 0.0,
        .pulsed = false,
        .finished = false,
    };
}

// NOTE: Repeating
void tick_sequence_timer(SequenceTimer* stimer, float delta_time_seconds) {
    if (stimer->checkpoint_count == 0) {
        stimer->finished = true;
        return;
    }

    switch (stimer->mode) {
    case Timer_NonRepeating:
        if (!stimer->finished) {
            float checkpoint = stimer->checkpoints[stimer->index];
            stimer->time_elapsed += delta_time_seconds;
            if (stimer->time_elapsed >= checkpoint) {
                stimer->pulsed = true;
                stimer->index++;
                stimer->index %= stimer->checkpoint_count;
                if (stimer->index == 0) {
                    stimer->finished = true;
                }
            }
        }
        break;
    case Timer_Repeating:
        float checkpoint = stimer->checkpoints[stimer->index];
        stimer->time_elapsed += delta_time_seconds;
        if (stimer->time_elapsed >= checkpoint) {
            stimer->pulsed = true;
            stimer->index++;
            stimer->index %= stimer->checkpoint_count;
            if (stimer->index == 0) {
                stimer->time_elapsed = 0.0;
            }
        }
        break;
    }
}

bool sequence_timer_has_pulsed(SequenceTimer* stimer) {
    if (stimer->pulsed) {
        stimer->pulsed = false;
        return true;
    }
    return false;
}

bool sequence_timer_is_finished(SequenceTimer* stimer) {
    switch (stimer->mode) {
    case Timer_NonRepeating:
        return stimer->finished;
    case Timer_Repeating:
        if (stimer->finished) {
            stimer->finished = false;
            return true;
        }
        return false;
    }
}

SpriteAnimation new_sprite_animation(SequenceTimer timer, TextureHandle* textures, int texture_count) {
    return (SpriteAnimation) {
        .timer = timer,
        .textures = textures,
        .texture_count = texture_count,
        .current_texture_ind = 0,
    };
}

void tick_animation_timer(SpriteAnimation* anim, float delta_time_seconds) {
    if (anim->texture_count == 0) return;

    tick_sequence_timer(&anim->timer, delta_time_seconds);
    if (sequence_timer_has_pulsed(&anim->timer)) {
        anim->current_texture_ind++;
        anim->current_texture_ind %= anim->texture_count;
    }
}

TextureHandle get_current_texture(SpriteAnimation* anim) {
    if (anim->texture_count == 0) return primary_texture_handle();
    return anim->textures[anim->current_texture_ind];
}