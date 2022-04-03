#include "stdio.h"
#include "stdlib.h"
#include "stdbool.h"
#include <string.h>

#include "emscripten.h"
#include "emscripten/fetch.h"


#include "SDL2/SDL.h"
#include <SDL2/SDL_render.h>


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

typedef struct {
    AssetStore asset_store;
    RenderData render_data;
} GameState;

void mainLoop(void *userdata) {

    GameState *game_state = (GameState*)userdata;

    // exit if all assets have not been downloaded
    for (int i = 0; i < game_state->asset_store.size; i++) {
        if (!game_state->asset_store.assets[i].completed) {
            return;
        }
    }

    SDL_Renderer *renderer = game_state->render_data.renderer;


    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_Rect r = {
        .x = 20,
        .y = 50,
        .w = 50,
        .h = 50,
    };

    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_RenderFillRect(renderer, &r);
    SDL_RenderPresent(renderer);
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


    for (int i = 0; i < (sizeof(asset_names) / sizeof(char *)); i++) {
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);


        //_ = fmt.bufPrint(attr.requestMethod[0..], "GET", .{}) catch return;
        sprintf(&attr.requestMethod, "GET");

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

    printf("Startinga asset downloads\n");
    game_state->asset_store = startAssetDownload();

    printf("Setting up window\n");
    SDL_Init(SDL_INIT_VIDEO);
    game_state->render_data.window = SDL_CreateWindow("test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1000, 1000, 0);
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
