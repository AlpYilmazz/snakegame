#include "stdbool.h"
#include "string.h"

#include "gameui.h"

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

bool Button_hovered(Button* button) {
    Vector2 mouse = GetMousePosition();
    return button->position.x <= mouse.x && mouse.x <= button->position.x + button->size.x
        && button->position.y <= mouse.y && mouse.y <= button->position.y + button->size.y;
}