#include "types.h"
#include <stdio.h>

Enemy create_enemy(Posf spawn_location) {
    return (Enemy){
        .entity =
            (Entity){
                .pos = spawn_location,
                .vel = (Posf){.x = 0, .y = 0},
                .health = 100.0f,
            },
        .spawn = spawn_location,
        .cooldown_next_state = 0,
        .cooldown_ticks = 0,
        .inverse_strafe = false,
        .state = StateIdle,
    };
}

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
    case PATH_END_UP:
        renderAsset(renderer, a, 16, 16, PATH_END_UP_X, PATH_END_UP_Y, r);
        break;
    case PATH_END_DOWN:
        renderAsset(renderer, a, 16, 16, PATH_END_DOWN_X, PATH_END_DOWN_Y, r);
        break;
    case PATH_END_LEFT:
        renderAsset(renderer, a, 16, 16, PATH_END_LEFT_X, PATH_END_LEFT_Y, r);
        break;
    case PATH_END_RIGHT:
        renderAsset(renderer, a, 16, 16, PATH_END_RIGHT_X, PATH_END_RIGHT_Y, r);
        break;
    case PATH_CORNER_DOWN_RIGHT:
        renderAsset(renderer, a, 16, 16, PATH_CORNER_DOWN_RIGHT_X,
                    PATH_CORNER_DOWN_RIGHT_Y, r);
        break;
    case PATH_CORNER_DOWN_LEFT:
        renderAsset(renderer, a, 16, 16, PATH_CORNER_DOWN_LEFT_X,
                    PATH_CORNER_DOWN_LEFT_Y, r);
        break;
    case PATH_CORNER_UP_RIGHT:
        renderAsset(renderer, a, 16, 16, PATH_CORNER_UP_RIGHT_X,
                    PATH_CORNER_UP_RIGHT_Y, r);
        break;
    case PATH_CORNER_UP_LEFT:
        renderAsset(renderer, a, 16, 16, PATH_CORNER_UP_LEFT_X,
                    PATH_CORNER_UP_LEFT_Y, r);
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

    // exit if multiplayer state hasn't completed
    if (game_state->multistate->sync_state == UNSYNCED) {
        return;
    } else if (game_state->multistate->sync_state == SYNC_FAILED) {
        return;
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

    // Move Enemy
    {
        Enemies *enemies = &game_state->enemies;
        Player player = game_state->player;
        Map map = game_state->map;

        for (size_t i = 0; i < enemies->num_enemies; i++) {
            Entity player_entity = {
                .pos = player.pos,
                .vel = player.vel,
            };

            Enemy *e = &enemies->enemies[i];

            enemy_update(e, &player_entity, map);
        }
    }

    // Render Map
    {
        Camera camera = game_state->camera;
        Map map = game_state->map;

        int block_w = BLOCK_WIDTH * camera.scale_x;
        int block_h = BLOCK_HEIGHT * camera.scale_y;

        int blocks_per_screen_w = (SCREEN_WIDTH / block_w);
        int blocks_per_screen_h = (SCREEN_HEIGHT / block_h);

        double top_left_x = camera.pos.x - ((float)blocks_per_screen_w / 2);
        double top_left_y = camera.pos.y - ((float)blocks_per_screen_h / 2);

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
            for (int y = 0; y < blocks_per_screen_h + 2; y++) {
                r.y = offset_y + (block_h * y);
                for (int x = 0; x < blocks_per_screen_w + 2; x++) {
                    r.x = offset_x + (block_w * x);
                    size_t block_index =
                        ((base_block_y + y) * map.width) + (base_block_x + x);

                    renderTile(renderer, map.blocks[block_index].type,
                               game_state->asset_store.assets[2], &r);
                }
            }
        }
    }

    // Draw enemy
    {
        Camera camera = game_state->camera;

        // check if enemy should be drawn on screen

        int block_w = BLOCK_WIDTH * camera.scale_x;
        int block_h = BLOCK_HEIGHT * camera.scale_y;

        int blocks_per_screen_w = (SCREEN_WIDTH / block_w);
        int blocks_per_screen_h = (SCREEN_HEIGHT / block_h);

        Posf camera_start = {.x = 0.0f, .y = 0.0f};
        Posf camera_end = {.x = 0.0f, .y = 0.0f};

        camera_start.x = camera.pos.x - ((float)blocks_per_screen_h / 2);
        camera_start.y = camera.pos.y - ((float)blocks_per_screen_w / 2);
        camera_end.x = camera.pos.x + ((float)blocks_per_screen_w / 2);
        camera_end.y = camera.pos.y + ((float)blocks_per_screen_h / 2);

        for (size_t i = 0; i < game_state->enemies.num_enemies; i++) {
            Enemy enemy = game_state->enemies.enemies[i];

            // if enemy is on screen
            if (camera_start.x <= enemy.entity.pos.x &&
                enemy.entity.pos.x <= camera_end.x &&
                camera_start.y <= enemy.entity.pos.y &&
                enemy.entity.pos.y <= camera_end.y) {

                float player_w = PLAYER_WIDTH * camera.scale_x;
                float player_h = PLAYER_WIDTH * camera.scale_y;

                Posf screen_pos = (Posf){
                    .x = (enemy.entity.pos.x - camera_start.x) * camera.scale_x,
                    .y = (enemy.entity.pos.y - camera_start.y) * camera.scale_y,
                };

                SDL_Rect r = {
                    .x = screen_pos.x,
                    .y = screen_pos.y,
                    .w = player_w,
                    .h = player_h,
                };

                SDL_Renderer *renderer = game_state->render_data.renderer;

                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &r);
            } else {
                continue;
            }
        }
    }

    // Draw player
    {
        Camera camera = game_state->camera;
        Player *player = &game_state->player;

        printf("player pos: %f %f\n", player->pos.x, player->pos.y);

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

int parseShrineFromJson(struct json_object_s *j, Shrine *s) {
    struct json_object_element_s *shrine_e = j->start;
    while (true) {
        if (shrine_e == NULL) {
            break;
        }
        if (strcmp(shrine_e->name->string, "ID") == 0) {
            printf("FOUND ID\n");
            struct json_number_s *num =
                (struct json_number_s *)shrine_e->value->payload;
            s->id = atoi(num->number);
        } else if (strcmp(shrine_e->name->string, "Pos") == 0) {
            struct json_number_s *num =
                (struct json_number_s *)shrine_e->value->payload;
            s->pos.x = atoi(num->number);
            s->pos.y = atoi(num->number);
        } else if (strcmp(shrine_e->name->string, "Power") == 0) {
            struct json_number_s *num =
                (struct json_number_s *)shrine_e->value->payload;
            s->power = atoi(num->number);
        } else if (strcmp(shrine_e->name->string, "State") == 0) {
            struct json_number_s *num =
                (struct json_number_s *)shrine_e->value->payload;
            s->state = atoi(num->number);
        } else if (strcmp(shrine_e->name->string, "CreatedBy") == 0) {
            struct json_object_s *obj =
                (struct json_object_s *)shrine_e->value->payload;
            struct json_object_element_s *obj_e = obj->start;
            struct json_string_s *str =
                (struct json_string_s *)obj_e->value->payload;
            s->created_by = strdup(str->string);
        } else if (strcmp(shrine_e->name->string, "Contributors") == 0) {
            struct json_array_s *arr =
                (struct json_array_s *)shrine_e->value->payload;
            s->contributors = malloc(sizeof(char *) * arr->length);
            s->contributors_len = arr->length;
            struct json_array_element_s *arr_e = arr->start;
            int i = 0;
            while (true) {
                if (arr_e == NULL) {
                    break;
                }
                struct json_object_s *arr_e_obj =
                    (struct json_object_s *)arr_e->value->payload;
                struct json_string_s *str =
                    (struct json_string_s *)arr_e_obj->start->value->payload;
                s->contributors[i] = strdup(str->string);
                i++;
                arr_e = arr_e->next;
            }
        }
        shrine_e = shrine_e->next;
    }
    return 0;
}
void stateDownloadSucceded(emscripten_fetch_t result) {

    printf("url: (%s)\n", result.url);

    MultiplayerStateSyncCallbackData *data =
        (MultiplayerStateSyncCallbackData *)result.userData;
    MultiplayerState *state = data->multistate;

    struct json_value_s *root = json_parse(result.data, result.numBytes);
    struct json_object_s *root_object = (struct json_object_s *)root->payload;

    struct json_object_element_s *shrines = root_object->start;

    struct json_array_s *shrines_array =
        (struct json_array_s *)shrines->value->payload;
    // create shrine buffer
    state->shrines_len = shrines_array->length;
    state->shrines = malloc(shrines_array->length * sizeof(Shrine));

    size_t i = 0;
    struct json_array_element_s *shrines_e = shrines_array->start;
    while (true) {
        if (shrines_e == NULL) {
            break;
        }

        struct json_object_s *shrines_e_obj =
            (struct json_object_s *)shrines_e->value->payload;
        if (parseShrineFromJson(shrines_e_obj, state->shrines + i) < 0) {
            state->sync_state = SYNC_FAILED;
            return;
        }

        shrines_e = shrines_e->next;
        i++;
    }

    printf("Found %lu shrines\n", state->shrines_len);
    for (size_t i = 0; i < state->shrines_len; i++) {
        printf("Shrines[%ld] {\n", i);
        printf("    \"ID\": %d,\n", state->shrines[i].id);
        printf("    \"Pos\": (%d, %d),\n", state->shrines[i].pos.x,
               state->shrines[i].pos.y);
        printf("    \"Power\": %d,\n", state->shrines[i].power);
        printf("    \"State\": %d,\n", state->shrines[i].state);
        printf("    CreatedBy: {\n");
        printf("        \"Username\": \"%s\"\n", state->shrines[i].created_by);
        printf("    },\n");
        printf("    Contributors: {\n");
        for (size_t j = 0; j < state->shrines[i].contributors_len; j++) {
            printf("        \"Username\": \"%s\"\n",
                   state->shrines[i].contributors[j]);
        }
        printf("    }\n");
        printf("}\n");
    }

    // TODO: add drawPaths call here once server is configured

    state->sync_state = SYNCED;
}

MultiplayerState *startStateDownload(Map *map) {

    MultiplayerStateSyncCallbackData *data =
        malloc(sizeof(MultiplayerStateSyncCallbackData));
    data->multistate = malloc(sizeof(MultiplayerState));
    *(data->multistate) = (MultiplayerState){
        .sync_state = UNSYNCED,
        .shrines = NULL,
        .shrines_len = 0,
    };
    data->map = map;

    char *state_file = "/res/shrine_list.json";

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);

    strcpy((char *)&attr.requestMethod, "GET");

    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

    attr.onsuccess = stateDownloadSucceded;
    attr.userData = data;

    emscripten_fetch(&attr, state_file);
    return (MultiplayerState *)attr.userData;
}

void drawPaths(Posi *shrines, size_t num_shrines, Block *blocks) {
    for (size_t i = 0; i < num_shrines; i++) {
        Posi nearest_shrines[1] = {0};
        size_t nearest_distances[1] = {SIZE_MAX};

        // find the nearest shrine
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
                Posi temp = start;
                start = end;
                end = temp;
            }

            // 50/50 change of UP -> RIGHT or RIGHT -> UP
            if ((start.x + end.x) % 2 == 0) {
                // plot y path
                if (start.y < end.y) {
                    blocks[((start.y + 1) * MAP_WIDTH) + start.x].type =
                        PATH_END_UP;
                    for (int y = start.y + 2; y <= end.y - 1; y++) {
                        blocks[y * MAP_WIDTH + start.x].type = PATH_VERT;
                    }
                    blocks[end.y * MAP_WIDTH + start.x].type =
                        PATH_CORNER_UP_RIGHT;
                } else if (start.y > end.y) {
                    blocks[((start.y - 1) * MAP_WIDTH) + start.x].type =
                        PATH_END_DOWN;
                    for (int y = start.y - 2; y >= end.y + 1; y--) {
                        blocks[y * MAP_WIDTH + start.x].type = PATH_VERT;
                    }
                    blocks[end.y * MAP_WIDTH + start.x].type =
                        PATH_CORNER_DOWN_RIGHT;
                }

                // plot x path
                for (int x = start.x + 1; x <= end.x - 2; x++) {
                    blocks[end.y * MAP_WIDTH + x].type = PATH_HORIZ;
                }
                blocks[end.y * MAP_WIDTH + (end.x - 1)].type = PATH_END_RIGHT;
            } else {
                // plot x path
                blocks[start.y * MAP_WIDTH + (start.x + 1)].type =
                    PATH_END_LEFT;
                for (int x = start.x + 2; x <= end.x - 1; x++) {
                    blocks[start.y * MAP_WIDTH + x].type = PATH_HORIZ;
                }

                // plot y path
                if (start.y < end.y) {
                    blocks[start.y * MAP_WIDTH + end.x].type =
                        PATH_CORNER_DOWN_LEFT;
                    for (int y = start.y + 1; y <= end.y - 2; y++) {
                        blocks[y * MAP_WIDTH + end.x].type = PATH_VERT;
                    }
                    blocks[((end.y - 1) * MAP_WIDTH) + end.x].type =
                        PATH_END_DOWN;
                } else if (start.y > end.y) {
                    blocks[start.y * MAP_WIDTH + end.x].type =
                        PATH_CORNER_UP_LEFT;
                    for (int y = start.y - 1; y >= end.y + 2; y--) {
                        blocks[y * MAP_WIDTH + end.x].type = PATH_VERT;
                    }
                    blocks[((end.y + 1) * MAP_WIDTH) + end.x].type =
                        PATH_END_UP;
                }
            }
        }
    }
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
    game_state->enemies = (Enemies){
        .enemies = malloc(sizeof(Enemy) * 1),
        .num_enemies = 1,
    };
    for (size_t i = 0; i < game_state->enemies.num_enemies; i++) {
        Posf spawn = {
            .x = 80.0f,
            .y = 10.0f,
        };
        game_state->enemies.enemies[i] = create_enemy(spawn);
    }

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

    drawPaths(shrines, num_shrines, blocks);

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

    printf("Starting game state downloads\n");
    game_state->multistate = startStateDownload(&game_state->map);

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

#define ENEMY_VELOCITY 0.13 // slightly faster than the player
#define ENEMY_ATTACK_DIST 0.5
#define ENEMY_RETREAT_DIST 5
#define ENEMY_AGRO_DIST 15
#define ENEMY_CHASE_DIST 25
#define ENEMY_ATTACK_COOLDOWN_TICKS 20
#define ENEMY_STRAFE_CHANCE 0.5f
#define ATTACK_WINDUP_TICKS 8
#define ENEMY_ATTACK_DAMAGE 20
#define ENEMY_KNOCKBACK_AMOUNT 1

bool random_bool(float true_chance) {
    int random = rand();
    float random_float = ((float)random) / ((float)RAND_MAX);
    return random_float < true_chance;
}

float posf_magnitute(Posf pos) { return sqrtf(pos.x * pos.x + pos.y * pos.y); }

Posf posf_reverse(Posf pos) {
    return (Posf){
        .x = -pos.x,
        .y = -pos.y,
    };
}

Posf posf_subtract(Posf a, Posf b) {
    return (Posf){
        .x = a.x - b.x,
        .y = a.y - b.y,
    };
}

Posf posf_add(Posf a, Posf b) {
    return (Posf){
        .x = a.x + b.x,
        .y = a.y + b.y,
    };
}

float posf_distance(Posf from, Posf to) {
    Posf dist_vec = posf_subtract(from, to);

    return posf_magnitute(dist_vec);
}

Posf posf_set_magnitute(Posf pos, float mag) {
    float current_mag = posf_magnitute(pos);
    float scale_factor = current_mag / mag;
    return (Posf){
        .x = pos.x / scale_factor,
        .y = pos.y / scale_factor,
    };
}

Posf posf_direction(Posf from, Posf to) {
    Posf out = posf_subtract(from, to);
    return posf_set_magnitute(out, 1);
}

void entity_move(Entity *e, Map map) {
    printf("entity vel vector (%f, %f)", e->vel.x, e->vel.y);
    if (e->vel.x < 0 && e->pos.x > 0) {
        e->pos.x += e->vel.x;
    } else if (e->vel.x > 0 && e->pos.x < map.width) {
        e->pos.x += e->vel.x;
    } else {
        // printf("BLOCKED e WITH POS: (%f, %f) and VEL: (%f, %f)\n",
        // e->pos.x, e->pos.y, e->vel.x, e->vel.y);
    }
    if (e->vel.y < 0 && e->pos.y > 0) {
        e->pos.y += e->vel.y;
    } else if (e->vel.y > 0 && e->pos.y < map.height) {
        e->pos.y += e->vel.y;
    }
}

void enemy_idle(Enemy *e, Entity *target) {

    Posf player_vec = posf_subtract(e->entity.pos, target->pos);
    float player_dist = posf_magnitute(player_vec);

    if (player_dist < ENEMY_AGRO_DIST) {
        e->state = StateChase;
        return;
    }
}

void enemy_return_to_spawn(Enemy *e, Entity *target) {
    float spawn_dist = posf_distance(e->spawn, e->entity.pos);
    if (spawn_dist < ENEMY_RETREAT_DIST) {
        e->state = StateIdle;
        return;
    }
    Posf spawn_direction = posf_direction(e->spawn, e->entity.pos);
    e->entity.vel = posf_set_magnitute(spawn_direction, ENEMY_VELOCITY);
}

void enemy_attack(Enemy *e, Entity *target) {
    e->entity.vel = (Posf){0, 0};
    // TODO: do attack
    if (posf_distance(target->pos, e->entity.pos)) {
        Posf attack_dir = posf_direction(target->pos, e->entity.pos);
        Posf attack_vec =
            posf_set_magnitute(attack_dir, ENEMY_KNOCKBACK_AMOUNT);
        target->pos = posf_add(target->pos, attack_vec);
        target->health -= ENEMY_ATTACK_DAMAGE;
    }

    e->cooldown_ticks = ENEMY_ATTACK_COOLDOWN_TICKS;
    e->cooldown_next_state = StateRetreat;
    e->state = StateCooldown;
}

void enemy_chase(Enemy *e, Entity *target) {

    float distance_to_spawn = posf_distance(e->entity.pos, e->spawn);
    if (distance_to_spawn > ENEMY_CHASE_DIST) {
        e->state = StateReturnToSpawn;
        return;
    }
    Posf player_vec = posf_subtract(target->pos, e->entity.pos);
    float player_dist = posf_magnitute(player_vec);
    printf("distance to player %f\n", player_dist);
    if (player_dist < ENEMY_ATTACK_DIST) {
        e->state = StateCooldown;
        e->cooldown_next_state = StateAttack;
        e->cooldown_ticks = ATTACK_WINDUP_TICKS;
        return;
    }
    Posf chase_vec = posf_set_magnitute(player_vec, ENEMY_VELOCITY);
    printf("chase vec magnitude %f\n", posf_magnitute(chase_vec));
    e->entity.vel = chase_vec;
}

void enemy_cooldown(Enemy *e, Entity *target) {
    e->cooldown_ticks -= 1;
    if (e->cooldown_ticks < 0) {
        e->state = e->cooldown_next_state;
    }
}

void enemy_retreat(Enemy *e, Entity *target) {
    Posf player_vec = posf_subtract(target->pos, e->entity.pos);
    float player_dist = posf_magnitute(player_vec);
    if (player_dist > ENEMY_RETREAT_DIST) {
        bool should_strafe = random_bool(ENEMY_STRAFE_CHANCE);
        if (should_strafe) {
            e->inverse_strafe = random_bool(0.5f);
            e->cooldown_ticks = 20;
            e->state = StateStrafe;
        } else {
            e->state = StateChase;
        }
        return;
    }
    Posf retreat_vec = posf_reverse(player_vec);
    e->entity.vel = posf_set_magnitute(retreat_vec, ENEMY_VELOCITY);
}

void enemy_strafe(Enemy *e, Entity *target) {
    e->cooldown_ticks -= 1;
    if (e->cooldown_ticks < 0) {
        e->state = StateRetreat;
        return;
    }
    Posf player_vec = posf_subtract(target->pos, e->entity.pos);
    Posf strafe_vec = {
        .x = player_vec.y,
        .y = -player_vec.x,
    };
    if (e->inverse_strafe) {
        strafe_vec = posf_reverse(strafe_vec);
    }
    e->entity.vel = posf_set_magnitute(strafe_vec, ENEMY_VELOCITY);
}

void enemy_update(Enemy *e, Entity *target, Map map) {
    printf("state is %d\n", e->state);
    switch (e->state) {
    case StateReturnToSpawn:
        enemy_return_to_spawn(e, target);
        break;
    case StateAttack:
        enemy_attack(e, target);
        break;
    case StateChase:
        enemy_chase(e, target);
        break;
    case StateStrafe:
        enemy_strafe(e, target);
        break;
    case StateRetreat:
        enemy_retreat(e, target);
        break;
    case StateIdle:
        enemy_idle(e, target);
        break;
    case StateCooldown:
        enemy_cooldown(e, target);
        break;
    }
    entity_move(e, map);
}
