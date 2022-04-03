#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "emscripten.h"
#include "emscripten/fetch.h"


#include "SDL2/SDL.h"
#include <SDL2/SDL_render.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000
#define MAP_WIDTH 200
#define MAP_HEIGHT 200

typedef struct {
    float x;
    float y;
} Pos;

typedef struct {
    char *path;
    bool completed;
    uint8_t *data;
} Asset;

typedef struct {
    Asset *assets;
    size_t size;
} AssetStore;

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
} RenderData;

typedef enum {
    GRASS,
    SAND,
} BlockType;

typedef struct {
  BlockType type;
} Block;

typedef struct {
    size_t height;
    size_t width;
    Block* blocks;
} Map;

typedef struct {
    Pos pos;
    float scale;
} Camera;

typedef struct {
   Pos pos;
} Player;

typedef struct {
    AssetStore asset_store;
    RenderData render_data;
    Map map;
    Camera camera;
} GameState;

void mainLoop(void *userdata) {

    GameState *game_state = (GameState*)userdata;

    // exit if all assets have not been downloaded
    for (size_t i = 0; i < game_state->asset_store.size; i++) {
        if (!game_state->asset_store.assets[i].completed) {
            return;
        }
    }


    // Render Map
    Camera camera = game_state->camera;
    Map map = game_state->map;

    int block_w = SCREEN_WIDTH * camera.scale;
    int block_h = SCREEN_HEIGHT * camera.scale;

    int blocks_per_screen_w = (SCREEN_WIDTH / block_w) + 1;
    int blocks_per_screen_h = (SCREEN_HEIGHT / block_h) + 1;

    double integral_x;
    double fractional_x = modf(camera.pos.x / block_w, &integral_x);
    double integral_y;
    double fractional_y = modf(camera.pos.y / block_h, &integral_y);


    int base_block_x = (int)integral_x;
    int offset_x = -(int)(fractional_x * block_w);
    int base_block_y = (int)integral_y;
    int offset_y = -(int)(fractional_y * block_h);


    SDL_Renderer *renderer = game_state->render_data.renderer;

    SDL_Rect r = {
        .x = 0,
        .y = 0,
        .w = block_w,
        .h = block_h,
    };
    for (int y = 0; y < blocks_per_screen_h; y++) {
        r.y = offset_y + (block_h * y);
        for (int x = 0; x < blocks_per_screen_w; x++) {
            r.x = offset_x + (block_w * x);
            size_t block_index = ((base_block_y  + y) * map.width) + (base_block_x + x);
            if (map.blocks[block_index].type == GRASS) {
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            } else {
                SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
            }
            SDL_RenderFillRect(renderer, &r);
        }
    }
    SDL_RenderPresent(renderer);



    //SDL_Renderer *renderer = game_state->render_data.renderer;



    //SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    //SDL_RenderClear(renderer);

    //SDL_Rect r = {
    //    .x = 20,
    //    .y = 50,
    //    .w = 50,
    //    .h = 50,
    //};

    //SDL_SetRnderDrawColor(renderer, 0, 255, 255, 255);
    //SDL_RenderFillRect(renderer, &r);
    //SDL_RenderPresent(renderer);
}

void downloadSucceededCallback(emscripten_fetch_t result) {
    Asset* asset = (Asset*) result.userData;
    printf("Asset Path: %s\n", asset->path);
    printf("Downloaded file: %s", result.url);
    printf(" (%ld bytes)\n", result.numBytes);

    asset->data = malloc(result.numBytes);
    for (size_t i = 0; i < result.numBytes; i++) {
        asset->data[i] = result.data[i];
    }
    asset->completed = true;

    emscripten_fetch_close(&result);

}

AssetStore startAssetDownload() {
    char *asset_names[] = {
        "/res/rpg16/default_grass.png",
        "/res/rpg16/default_sand.png",
    };
    size_t num_assets = sizeof(asset_names) / sizeof(char *);

    Asset* assets = malloc(sizeof(Asset) * (num_assets));


    for (size_t i = 0; i < (num_assets); i++) {
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);


        //_ = fmt.bufPrint(attr.requestMethod[0..], "GET", .{}) catch return;
        strcpy((char *)&attr.requestMethod, "GET");

        //attr.attributes = c.EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | c.EMSCRIPTEN_FETCH_SYNCHRONOUS;
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

        attr.onsuccess = downloadSucceededCallback;
        assets[i].path = strdup(asset_names[i]);
        attr.userData = assets + i;

        emscripten_fetch(&attr, asset_names[i]);

    }

    return (AssetStore) {
        .assets = assets,
        .size = num_assets,
    };
}


void main() {
    printf("Creating game state\n");
    GameState *game_state = malloc(sizeof(GameState));


    game_state->camera = (Camera) {
        .pos = (Pos) {
            .x = 25,
            .y = 25,
        },
        .scale = 0.01,
    };

    Block* blocks = malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(Block));
    for (size_t i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++) {
        if (((i + i / MAP_WIDTH) % 2) == 0) {
            blocks[i] = (Block) {
                .type = GRASS,
            };
        } else {
            blocks[i] = (Block) {
                .type = SAND,
            };
        }
    }

    game_state->map = (Map) {
        .width = MAP_WIDTH,
        .height = MAP_HEIGHT,
        .blocks = blocks,
    };


    printf("Startinga asset downloads\n");
    game_state->asset_store = startAssetDownload();

    printf("Setting up window\n");
    SDL_Init(SDL_INIT_VIDEO);
    game_state->render_data.window = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    game_state->render_data.renderer = SDL_CreateRenderer(game_state->render_data.window, -1, 0);

    int simulate_infinite_loop = 1; // call the function repeatedly
    int fps = -1; // call the functios as fast as the browser want to render (typically 60fps)

    printf("Creating main loop\n");
    emscripten_set_main_loop_arg(mainLoop, game_state, fps, simulate_infinite_loop);

    printf("Closing window\n");
    SDL_DestroyRenderer(game_state->render_data.renderer);
    SDL_DestroyWindow(game_state->render_data.window);
    SDL_Quit();
}
