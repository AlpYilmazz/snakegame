#ifndef SNAKEGAME_ANIMATION
#define SNAKEGAME_ANIMATION

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

#endif