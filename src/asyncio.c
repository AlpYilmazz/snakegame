#include <stdlib.h>
#include <stdio.h>

#include "raylib.h"

#include "threadpool.h"

#include "asset.h"

#include "asyncio.h"

void asyncio_load_texture_dir(TaskArgVoid* arg) {
    AsyncioLoadTextureDir* this_arg = arg;
    TextureAssets* texture_assets = this_arg->texture_assets;
    const char* dirpath = this_arg->dirpath;
    int* completed = this_arg->completed;
    int* handle_count = this_arg->handle_count;
    TextureHandle* handles = this_arg->handles;

    // FilePathList files = LoadDirectoryFilesEx(dirpath, "png", false);
    FilePathList files = LoadDirectoryFiles(dirpath);
    printf("[Asyncio] file count: %d\n", files.count);
    handles = malloc(files.count * sizeof(TextureHandle));
    for (int i = 0; i < files.count; i++) {
        handles[i] = reserve_texture_slot(texture_assets);
        printf("[Asyncio] %s -> %d\n", files.paths[i], handles[i].id);
    }
    for (int i = 0; i < files.count; i++) {
        Image img = LoadImage(files.paths[i]);
        Texture texture = LoadTextureFromImage(img); // TODO: impl ImageAssets, move texture creation to main thread
        put_texture(texture_assets, handles[i], texture);
    }
    *completed = 1;
    *handle_count = files.count;
}

Task get_task_asyncio_load_texture_dir(AsyncioLoadTextureDir* arg) {
    return (Task) {
        .handler = asyncio_load_texture_dir,
        .arg = arg,
    };
}