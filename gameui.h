#ifndef SNAKEGAME_GAMEUI
#define SNAKEGAME_GAMEUI

#include "stdbool.h"

#include "raylib.h"
#include "raymath.h"

typedef struct {
    int id;
    char title[21];
    char input[21];
    Vector2 position;
    Vector2 size;
    Color color;
    bool disabled;
} TextInput;

void TextInput_input(TextInput* text_input, char input);
void TextInput_delete_back(TextInput* text_input);
bool TextInput_hovered(TextInput* text_input);

typedef struct {
    int id;
    char title[20];
    Vector2 position;
    Vector2 size;
    Color color;
    bool disabled;
} Button;

bool Button_hovered(Button* button);

#endif