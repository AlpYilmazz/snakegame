#include <stdlib.h>
#include <stdio.h>
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

TextureResponse null_texture_response() {
    return (TextureResponse) { false, NULL };
}

TextureResponse valid_texture_response(Texture* texture) {
    return (TextureResponse) { true, texture };
}

TextureAssets new_texture_assets() {
    TextureAssets texture_assets = {0};

    Image image = {
        .data = (void*)&DEFAULT_IMAGE_DATA,
        .width = 1,
        .height = 1,
        .format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8,
        .mipmaps = 1
    };
    Texture texture = LoadTextureFromImage(image);

    texture_assets.next_slot_available_bump = 1;
    texture_assets.slots[PRIMARY_TEXTURE_HANDLE.id] = true;
    texture_assets.texture_ready[PRIMARY_TEXTURE_HANDLE.id] = true;
    texture_assets.images[PRIMARY_TEXTURE_HANDLE.id] = image;
    texture_assets.textures[PRIMARY_TEXTURE_HANDLE.id] = texture;
    
    return texture_assets;
}

TextureHandle texture_assets_reserve_texture_slot(TextureAssets* assets) {
    if (assets->next_slot_available_bump < TEXTURE_SLOTS) {
        assets->slots[assets->next_slot_available_bump] = true;
        return new_texture_handle(assets->next_slot_available_bump++);
    }
    for (int i = 0; i < TEXTURE_SLOTS; i++) {
        if (!assets->slots[i]) {
            assets->slots[i] = true;
            return new_texture_handle(i);
        }
    }
    // TODO: no texture slots available could not load texture
    return new_texture_handle(-1);
}

void texture_assets_put_image(TextureAssets* assets, TextureHandle handle, Image image) {
    assets->images[handle.id] = image;
}

void texture_assets_create_texture_uncheched(TextureAssets* assets, TextureHandle handle) {
    assets->textures[handle.id] = LoadTextureFromImage(assets->images[handle.id]);
    assets->texture_ready[handle.id] = true;
}

void texture_assets_put_image_and_create_texture(TextureAssets* assets, TextureHandle handle, Image image) {
    texture_assets_put_image(assets, handle, image);
    texture_assets_create_texture_uncheched(assets, handle);
}

TextureResponse texture_assets_get_texture(TextureAssets* assets, TextureHandle handle) {
    if (assets->texture_ready[handle.id]) {
        return valid_texture_response(&assets->textures[handle.id]);
    }
    return null_texture_response();
}

Texture* texture_assets_get_texture_or_default(TextureAssets* assets, TextureHandle handle) {
    if (assets->texture_ready[handle.id]) {
        return &assets->textures[handle.id];
    }
    return &assets->textures[PRIMARY_TEXTURE_HANDLE.id];
}

void texture_assets_unload_texture(TextureAssets* assets, TextureHandle handle) {
    assets->slots[handle.id] = false;
    assets->texture_ready[handle.id] = false;

    if (handle.id == assets->next_slot_available_bump - 1) {
        assets->next_slot_available_bump--;
    }

    Image image = assets->images[handle.id];
    Texture texture = assets->textures[handle.id];
    UnloadImage(image);
    UnloadTexture(texture);
    assets->images[handle.id] = (Image) {0};
    assets->textures[handle.id] = (Texture) {0};
}