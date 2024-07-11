#ifndef SNAKEGAME_ASSET
#define SNAKEGAME_ASSET

#include "stdbool.h"

#include "raylib.h"

#define TEXTURE_SLOTS 100

typedef struct {
    int id;
} TextureHandle;

TextureHandle new_texture_handle(int id);

typedef struct {
    bool exists;
    Texture* texture;
} TextureResponse;

TextureResponse null_response();
TextureResponse valid_response(Texture* texture);

typedef struct {
    Texture textures[TEXTURE_SLOTS];
    bool slots[TEXTURE_SLOTS];
    int next_slot_available_bump;
} TextureAssets;

TextureAssets new_texture_assets();
bool texture_exists(TextureAssets* assets, TextureHandle handle);
TextureResponse get_texture(TextureAssets* assets, TextureHandle handle);
Texture* get_texture_unchecked(TextureAssets* assets, TextureHandle handle);
TextureHandle reserve_texture_slot(TextureAssets* assets);
void put_texture(TextureAssets* assets, TextureHandle handle, Texture texture);
void remove_texture(TextureAssets* assets, TextureHandle handle);

#endif