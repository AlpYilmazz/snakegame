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

SequenceTimer new_sequence_timer(float* checkpoints, int count) {
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
    if (stimer->checkpoint_count == 0) return;

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

// void TextureList_push_back(TextureList* list, Texture item) {
//     #define INITIAL_CAPACITY 10
//     #define CAPACITY_GROWTH_FACTOR 2
    
//     if (list->capacity == 0) {
//         list->capacity = INITIAL_CAPACITY;
//         list->items = malloc(INITIAL_CAPACITY * sizeof(*&item));
//     }
//     else if (list->count == list->capacity) {
//         int new_capacity = CAPACITY_GROWTH_FACTOR * list->capacity;
//         list->items = realloc(list->items, new_capacity * sizeof(*&item));
//     }

//     list->items[list->count] = item;
//     list->count++;

//     #undef INITIAL_CAPACITY
//     #undef CAPACITY_GROWTH_FACTOR
// }

// TextureList load_texture_directory(const char* dir) {
//     TextureList texture_list = (TextureList) {0};

//     WIN32_FIND_DATA file_find_data;
//     HANDLE file_handle = NULL;
//     char path_spec[2048];

//     sprintf(path_spec, "%s\\*.png", dir);

//     if((file_handle = FindFirstFile(path_spec, &file_find_data)) == INVALID_HANDLE_VALUE) {
//         printf("Path not found: [%s]\n", dir);
//         return texture_list; // success := 0 (false)
//     }

//     do {
//         //Find first file will always return "."
//         //    and ".." as the first two directories.
//         if(strcmp(file_find_data.cFileName, ".") == 0 || strcmp(file_find_data.cFileName, "..") == 0) {
//             continue;
//         }
//         //Build up our file path using the passed in
//         //  [sDir] and the file/foldername we just found:
//         sprintf(path_spec, "%s\\%s", dir, file_find_data.cFileName);

//         //Is the entity a File or Folder?
//         if(file_find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
//             printf("Directory: %s\n", path_spec);
//         }
//         else {
//             printf("File: %s\n", path_spec);
//             TextureList_push_back(&texture_list, LoadTexture(path_spec));
//         }
//     } while(FindNextFile(file_handle, &file_find_data)); //Find the next file.

//     FindClose(file_handle);

//     texture_list.success = true;
//     return texture_list;
// }

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