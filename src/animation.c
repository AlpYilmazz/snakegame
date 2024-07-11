#include <stdio.h>
#include <stdbool.h>

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

SequenceTimer new_sequence_timer(float* checkpoints, int count) {
    printf("ch[0]: %0.2f\n", checkpoints[0]);
    return (SequenceTimer) {
        .checkpoint_count = count,
        .index = 0,
        .checkpoints = checkpoints,
        .time_elapsed = 0.0,
        .pulsed = false,
    };
}

// NOTE: Repeating
void tick_sequence_timer(SequenceTimer* stimer, float delta_time_seconds) {
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
}

bool sequence_timer_has_pulsed(SequenceTimer* stimer) {
    if (stimer->pulsed) {
        stimer->pulsed = false;
        return true;
    }
    return false;
}