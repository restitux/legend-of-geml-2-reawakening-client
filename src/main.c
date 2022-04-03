#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL2/SDL_events.h"
#include "SDL2/SDL_keycode.h"
#include "emscripten.h"
#include "emscripten/fetch.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rwops.h>
#include <SDL2/SDL_surface.h>

#define SCREEN_WIDTH 1000
#define SCREEN_HEIGHT 1000

#define MAP_WIDTH 500
#define MAP_HEIGHT 500

#define BLOCK_WIDTH 1
#define BLOCK_HEIGHT 1

#define PLAYER_WIDTH 1
#define PLAYER_HEIGHT 1

#define DEFAULT_BLOCKS_PER_SCREEN_W 20.0
#define DEFAULT_BLOCKS_PER_SCREEN_H 20.0
#define CAMERA_SCALE_DEFAULT_W 50
#define CAMERA_SCALE_DEFAULT_H 50
//#define DEFAULT_BLOCK_W SCREEN_WIDTH / DEFAULT_BLOCKS_PER_SCREEN_W
//#define DEFAULT_BLOCK_H SCREEN_WIDTH / DEFAULT_BLOCKS_PER_SCREEN_H
//#define CAMERA_SCALE_DEFAULT_W
//#define CAMERA_SCALE_DEFAULT_H 1.0 / DEFAULT_BLOCK_H

#define CAMERA_MAX_SCROLL 4.0
#define CAMERA_SCROLL_INC CAMERA_MAX_SCROLL / 10.0

#define PLAYER_VELOCTY 0.04

typedef struct {
    int x;
    int y;
} Posi;

typedef struct {
    float x;
    float y;
} Posf;

typedef struct {
    char *path;
    bool completed;
    SDL_Texture *texture;
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
    PATH_VERT,
    PATH_HORIZ,
    PATH_END_TOP,
    PATH_END_BOT,
    PATH_END_LEFT,
    PATH_END_RIGHT,
    SHRINE,
} BlockType;

#define GRASS_X 0
#define GRASS_Y 0
#define PATH_VERT_X 4
#define PATH_VERT_Y 2
#define PATH_HORIZ_X 6
#define PATH_HORIZ_Y 0
#define PATH_END_TOP_X 4
#define PATH_END_TOP_Y 1
#define PATH_END_BOT_X 4
#define PATH_END_BOT_Y 3
#define PATH_END_LEFT_X 5
#define PATH_END_LEFT_Y 0
#define PATH_END_RIGHT_X 7
#define PATH_END_RIGHT_Y 0
#define SHRINE_X 6
#define SHRINE_Y 16

typedef struct {
    BlockType type;
} Block;

typedef struct {
    size_t height;
    size_t width;
    Block *blocks;
} Map;

typedef struct {
    Posf pos;
    float scale_x;
    float scale_y;
    Posf scroll;
} Camera;

typedef struct {
    Posf pos;
    Posf vel;
} Player;

typedef struct {
    AssetStore asset_store;
    RenderData render_data;
    Map map;
    Camera camera;
    Player player;
} GameState;

typedef struct {
    size_t asset_index;
    AssetStore asset_store;
    RenderData render_data;
} AssetDownloadCallbackData;

void renderAsset(SDL_Renderer *renderer, Asset a, int w, int h, int x, int y,
                 SDL_Rect *dst) {
    SDL_RenderCopy(renderer, a.texture,
                   &(SDL_Rect){
                       .w = w,
                       .h = h,
                       .x = x * w,
                       .y = y * h,
                   },
                   dst);
}

void renderTile(SDL_Renderer *renderer, BlockType type, Asset a, SDL_Rect *r) {
    switch (type) {
    case GRASS:
        renderAsset(renderer, a, 16, 16, GRASS_X, GRASS_Y, r);
        break;
    case PATH_VERT:
        renderAsset(renderer, a, 16, 16, PATH_VERT_X, PATH_VERT_Y, r);
        break;
    case PATH_HORIZ:
        renderAsset(renderer, a, 16, 16, PATH_HORIZ_X, PATH_HORIZ_Y, r);
        break;
    case PATH_END_TOP:
        renderAsset(renderer, a, 16, 16, PATH_END_TOP_X, PATH_END_TOP_Y, r);
        break;
    case PATH_END_BOT:
        renderAsset(renderer, a, 16, 16, PATH_END_BOT_X, PATH_END_BOT_Y, r);
        break;
    case PATH_END_LEFT:
        renderAsset(renderer, a, 16, 16, PATH_END_LEFT_X, PATH_END_LEFT_Y, r);
        break;
    case PATH_END_RIGHT:
        renderAsset(renderer, a, 16, 16, PATH_END_RIGHT_X, PATH_END_RIGHT_Y, r);
        break;
    case SHRINE:
        renderAsset(renderer, a, 16, 16, SHRINE_X, SHRINE_Y, r);
        break;
    default:
        break;
    }
}

void processInput(Player *player, Camera *camera, SDL_Event *e) {
    if (e->type == SDL_KEYDOWN) {
        switch (e->key.keysym.sym) {
        case SDLK_UP:
            player->vel.y = -PLAYER_VELOCTY;
            break;
        case SDLK_DOWN:
            player->vel.y = PLAYER_VELOCTY;
            break;
        case SDLK_LEFT:
            player->vel.x = -PLAYER_VELOCTY;
            break;
        case SDLK_RIGHT:
            player->vel.x = PLAYER_VELOCTY;
            break;
        case SDLK_f:
            camera->scale_x++;
            camera->scale_y++;
            break;
        case SDLK_u:
            camera->scale_x--;
            camera->scale_y--;
            break;
        default:
            break;
        }
    } else if (e->type == SDL_KEYUP) {
        switch (e->key.keysym.sym) {
        case SDLK_UP:
            player->vel.y = 0;
            break;
        case SDLK_DOWN:
            player->vel.y = 0;
            break;
        case SDLK_LEFT:
            player->vel.x = 0;
            break;
        case SDLK_RIGHT:
            player->vel.x = 0;
            break;
        default:
            break;
        }
    }
}

void mainLoop(void *userdata) {

    GameState *game_state = (GameState *)userdata;

    // exit if all assets have not been downloaded
    for (size_t i = 0; i < game_state->asset_store.size; i++) {
        if (!game_state->asset_store.assets[i].completed) {
            return;
        }
    }

    // Process Input + Movement
    {
        Player *player = &game_state->player;
        Camera *camera = &game_state->camera;
        Map map = game_state->map;

        SDL_Event e;

        while (SDL_PollEvent(&e) != 0) {
            processInput(player, camera, &e);
        }

        if (player->vel.x < 0 && player->pos.x > 0) {
            player->pos.x += player->vel.x;
        } else if (player->vel.x > 0 && player->pos.x < map.width) {
            player->pos.x += player->vel.x;
        } else {
            // printf("BLOCKED PLAYER WITH POS: (%f, %f) and VEL: (%f, %f)\n",
            // player->pos.x, player->pos.y, player->vel.x, player->vel.y);
        }
        if (player->vel.y < 0 && player->pos.y > 0) {
            player->pos.y += player->vel.y;
        } else if (player->vel.y > 0 && player->pos.y < map.height) {
            player->pos.y += player->vel.y;
        }
    }

    // Move camera
    {
        Camera *camera = &game_state->camera;
        Player player = game_state->player;

        if (player.vel.x > 0) {
            if (camera->scroll.x <= CAMERA_MAX_SCROLL) {
                camera->scroll.x += CAMERA_SCROLL_INC;
            }
        } else if (player.vel.x < 0) {
            if (camera->scroll.x >= -CAMERA_MAX_SCROLL) {
                camera->scroll.x -= CAMERA_SCROLL_INC;
            }
        } else {
            if (camera->scroll.x > 0) {
                camera->scroll.x -= CAMERA_SCROLL_INC;
            } else if (camera->scroll.x < 0) {
                camera->scroll.x += CAMERA_SCROLL_INC;
            }
        }

        if (player.vel.y > 0) {
            if (camera->scroll.y <= CAMERA_MAX_SCROLL) {
                camera->scroll.y += CAMERA_SCROLL_INC;
            }
        } else if (player.vel.y < 0) {
            if (camera->scroll.y >= -CAMERA_MAX_SCROLL) {
                camera->scroll.y -= CAMERA_SCROLL_INC;
            }
        } else {
            if (camera->scroll.y > 0) {
                camera->scroll.y -= CAMERA_SCROLL_INC;
            } else if (camera->scroll.y < 0) {
                camera->scroll.y += CAMERA_SCROLL_INC;
            }
        }

        camera->pos.x = player.pos.x; // + camera->scroll.x;
        camera->pos.y = player.pos.y; // + camera->scroll.y;
    }

    // Render Map
    {
        Camera camera = game_state->camera;
        Map map = game_state->map;

        int block_w = BLOCK_WIDTH * camera.scale_x;
        int block_h = BLOCK_WIDTH * camera.scale_y;

        int blocks_per_screen_w = (SCREEN_WIDTH / block_w) + 1;
        int blocks_per_screen_h = (SCREEN_HEIGHT / block_h) + 1;

        double top_left_x = camera.pos.x - ((float)blocks_per_screen_w / 2) + 2;
        double top_left_y = camera.pos.y - ((float)blocks_per_screen_h / 2) + 2;

        double integral_x;
        double fractional_x = modf(top_left_x, &integral_x);
        double integral_y;
        double fractional_y = modf(top_left_y, &integral_y);

        int base_block_x = (int)integral_x;
        int offset_x = -(int)(fractional_x * block_w);
        int base_block_y = (int)integral_y;
        int offset_y = -(int)(fractional_y * block_h);

        {
            SDL_Renderer *renderer = game_state->render_data.renderer;

            // clear screen to light blue
            SDL_SetRenderDrawColor(renderer, 102, 230, 255, 255);
            SDL_RenderClear(renderer);

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
                    size_t block_index =
                        ((base_block_y + y) * map.width) + (base_block_x + x);

                    renderTile(renderer, map.blocks[block_index].type,
                               game_state->asset_store.assets[2], &r);
                }
            }
        }
    }

    // Draw player
    {
        Camera camera = game_state->camera;
        Player *player = &game_state->player;

        float player_w = PLAYER_WIDTH * camera.scale_x;
        float player_h = PLAYER_WIDTH * camera.scale_y;

        SDL_Rect r = {
            .x = ((float)SCREEN_WIDTH / 2) -
                 (player_w / 2), // - (camera.scroll.x * player_w),
            .y = ((float)SCREEN_HEIGHT / 2) -
                 (player_h / 2), // - (camera.scroll.y * player_h),
            .w = player_w,
            .h = player_h,
        };

        SDL_Renderer *renderer = game_state->render_data.renderer;

        SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
        SDL_RenderFillRect(renderer, &r);
    }

    SDL_RenderPresent(game_state->render_data.renderer);
}

void downloadSucceededCallback(emscripten_fetch_t result) {
    AssetDownloadCallbackData *data =
        (AssetDownloadCallbackData *)result.userData;
    Asset *asset = data->asset_store.assets + data->asset_index;

    printf("Downloaded file: %s", result.url);
    printf(" (%ld bytes)\n", result.numBytes);

    // load image data as SDL surface
    SDL_RWops *rw = SDL_RWFromMem((void *)result.data, result.numBytes);
    SDL_Surface *temp = IMG_Load_RW(rw, 1);
    if (temp == NULL) {
        printf("Image %s failed to load: %s\n", asset->path, IMG_GetError());
    } else {
        printf("Image %s loaded sucessfully\n", asset->path);
    }

    // Convert SDL_Surface to SDL_Texture
    asset->texture =
        SDL_CreateTextureFromSurface(data->render_data.renderer, temp);
    if (asset->texture == NULL) {
        fprintf(stderr, "CreateTextureFromSurface for image %s failed: %s\n",
                asset->path, SDL_GetError());
    } else {
        printf("Converted image %s to texture successfully\n", asset->path);
    }

    SDL_FreeSurface(temp);

    // flag asset as downloaded
    asset->completed = true;

    // close http connection
    emscripten_fetch_close(&result);
}

AssetStore startAssetDownload(RenderData render_data) {
    char *asset_names[] = {
        "/res/rpg16/default_grass.png",
        "/res/rpg16/default_sand.png",
        "/res/TilesetGrass/overworld_tileset_grass.png",
    };
    size_t num_assets = sizeof(asset_names) / sizeof(char *);

    Asset *assets = malloc(sizeof(Asset) * (num_assets));

    AssetStore asset_store = {
        .assets = assets,
        .size = num_assets,
    };

    for (size_t i = 0; i < (num_assets); i++) {
        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);

        strcpy((char *)&attr.requestMethod, "GET");

        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

        attr.onsuccess = downloadSucceededCallback;
        assets[i].path = strdup(asset_names[i]);
        attr.userData = malloc(sizeof(AssetDownloadCallbackData));
        *((AssetDownloadCallbackData *)attr.userData) =
            (AssetDownloadCallbackData){
                .asset_index = i,
                .asset_store = asset_store,
                .render_data = render_data,
            };

        emscripten_fetch(&attr, asset_names[i]);
    }

    return asset_store;
}

int main() {
    printf("Creating game state\n");
    GameState *game_state = malloc(sizeof(GameState));

    game_state->player = (Player){.pos =
                                      (Posf){
                                          .x = 100.0,
                                          .y = 100.0,
                                      },
                                  .vel = (Posf){
                                      .x = 0.0,
                                      .y = 0.0,
                                  }};

    game_state->camera = (Camera){
        .pos =
            (Posf){
                .x = 0.0,
                .y = 0.0,
            },
        .scale_x = CAMERA_SCALE_DEFAULT_W,
        .scale_y = CAMERA_SCALE_DEFAULT_H,
        .scroll =
            (Posf){
                .x = 0.0,
                .y = 0.0,
            },

    };

    Block *blocks = malloc(MAP_WIDTH * MAP_HEIGHT * sizeof(Block));
    for (size_t i = 0; i < MAP_WIDTH * MAP_HEIGHT; i++) {
        blocks[i] = (Block){
            .type = GRASS,
        };
    }

    size_t buffer_size = 10;
    Posi *shrines = malloc(sizeof(Posi) * buffer_size);
    size_t num_shrines = 0;

    int max_step = 5000;
    int rand_num = 0;
    srand(time(0));
    for (int i = 0; i < MAP_WIDTH * MAP_HEIGHT;) {
        rand_num = rand();
        float rand_f = (float)rand_num / (float)RAND_MAX;
        rand_num = rand_f * max_step;

        i += rand_num;

        blocks[i] = (Block){.type = SHRINE};

        if (buffer_size == num_shrines) {
            buffer_size *= 2;
            shrines = realloc(shrines, sizeof(Posi) * buffer_size);
        }
        shrines[num_shrines] = (Posi){
            .x = i % MAP_WIDTH,
            .y = (int)((float)i / MAP_WIDTH),
        };
        // printf("shrines[%lu] (%p): (%d, %d)\n", num_shrines, (void*)(shrines
        // + num_shrines), shrines[num_shrines].x, shrines[num_shrines].y);
        num_shrines++;
    }

    for (size_t i = 0; i < num_shrines; i++) {
        Posi nearest_shrines[1] = {0};
        size_t nearest_distances[1] = {SIZE_MAX};

        // find the 2 nearest shrines
        for (size_t j = 0; j < num_shrines; j++) {
            if (i == j) {
                continue;
            }

            // calculate manhattan distance
            size_t distance = abs(shrines[i].x - shrines[j].x) +
                              abs(shrines[i].y - shrines[j].y);
            for (size_t k = 0; k < 1; k++) {
                if (distance <= nearest_distances[k]) {
                    // for (size_t l = 1; l > k; l--) {
                    //     nearest_distances[l] = nearest_distances[l - 1];
                    //     nearest_shrines[l] = nearest_shrines[l - 1];
                    // }
                    nearest_distances[k] = distance;
                    nearest_shrines[k] = shrines[j];
                    break;
                }
            }
        }

        // draw paths to nearest shrines
        // first pass, dumb drawing algorithm
        for (size_t j = 0; j < 1; j++) {
            Posi start = shrines[i];
            Posi end = nearest_shrines[j];

            // ignore this annyoing case for now
            if (start.x == end.x || start.y == end.y) {
                continue;
            }

            if (start.x > end.x) {
                blocks[start.y * MAP_WIDTH + (start.x - 1)] = (Block){
                    .type = PATH_END_RIGHT,
                };
                for (int x = start.x - 2; x >= end.x + 1; x--) {
                    blocks[start.y * MAP_WIDTH + x] = (Block){
                        .type = PATH_HORIZ,
                    };
                }
                if (start.y > end.y) {
                    blocks[start.y * MAP_WIDTH + (end.x)] = (Block){
                        .type = PATH_END_RIGHT,
                    };

                } else if (end.y < start.y) {
                    blocks[start.y * MAP_WIDTH + (start.x - 1)] = (Block){
                        .type = PATH_END_RIGHT,
                    };
                }
            } else if (start.x < end.x) {
                blocks[start.y * MAP_WIDTH + (start.x + 1)] = (Block){
                    .type = PATH_END_LEFT,
                };
                for (int x = start.x + 2; x < end.x - 1; x++) {
                    blocks[start.y * MAP_WIDTH + x] = (Block){
                        .type = PATH_HORIZ,
                    };
                }
            }

            if (start.y > end.y) {
                for (int y = start.y - 1; y > end.y + 1; y--) {
                    blocks[y * MAP_WIDTH + end.x] = (Block){
                        .type = PATH_VERT,
                    };
                }
                blocks[(end.y + 1) * MAP_WIDTH + end.x] = (Block){
                    .type = PATH_END_TOP,
                };

            } else if (start.y < end.y) {
                for (int y = start.y + 1; y < end.y - 1; y++) {
                    blocks[y * MAP_WIDTH + end.x] = (Block){
                        .type = PATH_VERT,
                    };
                }
                blocks[(end.y - 1) * MAP_WIDTH + end.x] = (Block){
                    .type = PATH_END_BOT,
                };
            }
        }
    }

    game_state->map = (Map){
        .width = MAP_WIDTH,
        .height = MAP_HEIGHT,
        .blocks = blocks,
    };

    printf("Setting up window\n");
    SDL_Init(SDL_INIT_VIDEO);
    game_state->render_data.window = SDL_CreateWindow(
        "test", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
        SCREEN_HEIGHT, 0);
    game_state->render_data.renderer =
        SDL_CreateRenderer(game_state->render_data.window, -1, 0);

    printf("Starting asset downloads\n");
    game_state->asset_store = startAssetDownload(game_state->render_data);

    int simulate_infinite_loop = 1; // call the function repeatedly
    int fps = -1; // call the functios as fast as the browser want to render
                  // (typically 60fps)

    printf("Creating main loop\n");
    emscripten_set_main_loop_arg(mainLoop, game_state, fps,
                                 simulate_infinite_loop);

    printf("Closing window\n");
    SDL_DestroyRenderer(game_state->render_data.renderer);
    SDL_DestroyWindow(game_state->render_data.window);
    SDL_Quit();
}
