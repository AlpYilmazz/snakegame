#ifndef SNAKEGAME_ASYNCIO
#define SNAKEGAME_ASYNCIO

#include "threadpool.h"

#include "asset.h"

typedef struct {
    TextureAssets* texture_assets;
    const char* dirpath;
    int* completed_event;
    int* handle_count;
    TextureHandle** handles;
} AsyncioLoadTextureDir;

Task get_task_asyncio_load_texture_dir(AsyncioLoadTextureDir* arg);

#endif