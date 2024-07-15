#ifndef SNAKEGAME_ASSET
#define SNAKEGAME_ASSET

#include <stdbool.h>

#include "raylib.h"

#define TEXTURE_SLOTS 100

typedef struct {
    int id;
} TextureHandle;

TextureHandle primary_texture_handle();
TextureHandle new_texture_handle(int id);

typedef struct {
    bool exists;
    Texture* texture;
} TextureResponse;

TextureResponse null_texture_response();
TextureResponse valid_texture_response(Texture* texture);

typedef struct {
    int next_slot_available_bump;
    bool slots[TEXTURE_SLOTS];
    bool texture_ready[TEXTURE_SLOTS];
    Image images[TEXTURE_SLOTS];
    Texture textures[TEXTURE_SLOTS];
} TextureAssets;

TextureAssets new_texture_assets();
TextureHandle texture_assets_reserve_texture_slot(TextureAssets* assets);
void texture_assets_put_image(TextureAssets* assets, TextureHandle handle, Image image);
void texture_assets_create_texture_uncheched(TextureAssets* assets, TextureHandle handle);
void texture_assets_put_image_and_create_texture(TextureAssets* assets, TextureHandle handle, Image image);
TextureResponse texture_assets_get_texture(TextureAssets* assets, TextureHandle handle);
Texture* texture_assets_get_texture_or_default(TextureAssets* assets, TextureHandle handle);
void texture_assets_unload_texture(TextureAssets* assets, TextureHandle handle);

#endif