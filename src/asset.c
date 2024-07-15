#include <stdlib.h>
#include <stdbool.h>

#include "raylib.h"

#include "asset.h"

const TextureHandle PRIMARY_TEXTURE_HANDLE = {0};
const unsigned char DEFAULT_IMAGE_DATA[4] = {255, 0, 255, 255};

TextureHandle primary_texture_handle() {
    return PRIMARY_TEXTURE_HANDLE;
}

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
    TextureAssets texture_assets = {
        .textures = {0},
        .slots = {0},
        .next_slot_available_bump = 0,
    };

    Image img = {
        .data = (void*)&DEFAULT_IMAGE_DATA,
        .width = 1,
        .height = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };
    Texture texture = LoadTextureFromImage(img);
    put_texture(&texture_assets, PRIMARY_TEXTURE_HANDLE, texture);
    texture_assets.next_slot_available_bump++;
    
    return texture_assets;
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