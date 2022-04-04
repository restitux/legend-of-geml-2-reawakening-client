#ifndef TYPES_H
#define TYPES_H

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

#include "json.h"

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

#define PLAYER_VELOCTY 0.12
#define PLAYER_KNOCKBACK_AMOUNT 10
#define PLAYER_ATTACK_RANGE 2

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
    PATH_END_UP,
    PATH_END_DOWN,
    PATH_END_LEFT,
    PATH_END_RIGHT,
    PATH_CORNER_DOWN_RIGHT,
    PATH_CORNER_DOWN_LEFT,
    PATH_CORNER_UP_RIGHT,
    PATH_CORNER_UP_LEFT,
    SHRINE,
} BlockType;

typedef enum {
    ANIMATION_FRAME_UP0 = 0,
    ANIMATION_FRAME_UP1 = 1,
    ANIMATION_FRAME_UP2 = 2,
    ANIMATION_FRAME_RIGHT0 = 3,
    ANIMATION_FRAME_RIGHT1 = 4,
    ANIMATION_FRAME_RIGHT2 = 5,
    ANIMATION_FRAME_DOWN0 = 6,
    ANIMATION_FRAME_DOWN1 = 7,
    ANIMATION_FRAME_DOWN2 = 8,
    ANIMATION_FRAME_LEFT0 = 9,
    ANIMATION_FRAME_LEFT1 = 10,
    ANIMATION_FRAME_LEFT2 = 11,
} AnimationFrame;

#define GRASS_X 0
#define GRASS_Y 0
#define PATH_VERT_X 4
#define PATH_VERT_Y 2
#define PATH_HORIZ_X 6
#define PATH_HORIZ_Y 0
#define PATH_END_UP_X 4
#define PATH_END_UP_Y 1
#define PATH_END_DOWN_X 4
#define PATH_END_DOWN_Y 3
#define PATH_END_LEFT_X 5
#define PATH_END_LEFT_Y 0
#define PATH_END_RIGHT_X 7
#define PATH_END_RIGHT_Y 0
#define PATH_CORNER_DOWN_RIGHT_X 5
#define PATH_CORNER_DOWN_RIGHT_Y 1
#define PATH_CORNER_DOWN_LEFT_X 7
#define PATH_CORNER_DOWN_LEFT_Y 1
#define PATH_CORNER_UP_RIGHT_X 5
#define PATH_CORNER_UP_RIGHT_Y 3
#define PATH_CORNER_UP_LEFT_X 7
#define PATH_CORNER_UP_LEFT_Y 3
#define SHRINE_X 6
#define SHRINE_Y 16

typedef enum {
    ENEMY_TYPE_SKELETON,
    ENEMY_TYPE_ZOMBIE,
} EnemyType;

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
    bool animating;
    AnimationFrame f;
    size_t counter;
    size_t rate;
    Asset *asset;
    size_t bases[4];
    int w;
    int h;
    int w_offset;
    int h_offset;
    int x_offset;
    int y_offset;
} Animation;

typedef struct {
    Posf pos;
    Posf vel;
    float health;
    Posf facing;
    Animation animation;
} Player;

typedef struct {
    Posf pos;
    Posf vel;
    float health;
    Animation animation;
} Entity;

typedef struct {
    Entity entity; // player's repr in the world
} NewPlayer;

typedef enum {
    StateIdle,
    StateChase,
    StateAttack,
    StateCooldown, // used for any cooldown. Set cooldown_ticks and
                   // cooldown_next_state
    StateReturnToSpawn,
    StateStrafe,
    StateRetreat,
} FightingAIState;

typedef struct {
    Entity entity;
    FightingAIState state;
    Posf spawn;
    int cooldown_ticks;
    FightingAIState cooldown_next_state;
    bool inverse_strafe;
} Enemy;

typedef struct {
    Enemy *enemies;
    size_t num_enemies;
} Enemies;

void enemy_update(Enemy *e, Entity *target, Map map);

typedef enum {
    UNSYNCED,
    SYNCED,
    SYNC_FAILED,
} MULTIPLAYER_SYNC_STATE;

typedef struct {
    int id;
    Posi pos;
    int power;
    int state;
    char *created_by;
    char **contributors;
    size_t contributors_len;
} Shrine;

typedef struct {
    MULTIPLAYER_SYNC_STATE sync_state;
    Shrine *shrines;
    size_t shrines_len;
} MultiplayerState;

typedef struct {
    AssetStore asset_store;
    RenderData render_data;
    Map map;
    Camera camera;
    Player player;
    MultiplayerState *multistate;
    Enemies enemies;
} GameState;

typedef struct {
    size_t asset_index;
    AssetStore asset_store;
    RenderData render_data;
} AssetDownloadCallbackData;

typedef struct {
    Map *map;
    MultiplayerState *multistate;
} MultiplayerStateSyncCallbackData;

Posf posf_world_to_camera(Posf posf, Camera camera) {
    Posf camera_pos = camera.pos;
    float scale_x = camera.scale_x;
    float scale_y = camera.scale_y;

    float screen_x = (posf.x - camera_pos.x) * scale_x;
    float screen_y = (posf.y - camera_pos.y) * scale_y;

    return (Posf){
        .x = screen_x,
        .y = screen_y,
    };
}

#endif
