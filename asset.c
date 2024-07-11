#include "stdlib.h"
#include "stdbool.h"

#include "raylib.h"

#include "asset.h"

TextureHandle new_texture_handle(int id) {
    return (TextureHandle) { id };
}

TextureResponse null_response() {
    return (TextureResponse) { false, NULL };
}

TextureResponse valid_response(Texture* texture) {
    return (TextureResponse) { true, texture };
}

TextureAssets new_texture_assets() {
    return (TextureAssets) {
        .textures = {0},
        .slots = {0},
        .next_slot_available_bump = 0,
    };
}

bool texture_exists(TextureAssets* assets, TextureHandle handle) {
    return assets->slots[handle.id];
}

TextureResponse get_texture(TextureAssets* assets, TextureHandle handle) {
    if (assets->slots[handle.id]) {
        return valid_response(&assets->textures[handle.id]);
    }
    return null_response();
}

Texture* get_texture_unchecked(TextureAssets* assets, TextureHandle handle) {
    return &assets->textures[handle.id];
}

TextureHandle reserve_texture_slot(TextureAssets* assets) {
    if (assets->next_slot_available_bump < TEXTURE_SLOTS) {
        assets->slots[assets->next_slot_available_bump] = true;
        return new_texture_handle(assets->next_slot_available_bump++);
    }
    for (int i = 0; i < TEXTURE_SLOTS; i++) {
        if (!assets->slots[i]) {
            assets->slots[assets->next_slot_available_bump] = true;
            return new_texture_handle(i);
        }
    }
    // TODO: no texture slots available could not load texture
    return new_texture_handle(-1);
}

void put_texture(TextureAssets* assets, TextureHandle handle, Texture texture) {
    assets->slots[handle.id] = true;
    assets->textures[handle.id] = texture;
}

void remove_texture(TextureAssets* assets, TextureHandle handle) {
    assets->slots[handle.id] = false;
    if (handle.id == assets->next_slot_available_bump - 1) {
        assets->next_slot_available_bump--;
    }
    Texture texture = assets->textures[handle.id];
    UnloadTexture(texture);
    assets->textures[handle.id] = (Texture) {0};
}