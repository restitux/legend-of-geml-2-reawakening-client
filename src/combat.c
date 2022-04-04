#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define ENEMY_VELOCITY 0.13 // slightly faster than the player
#define ENEMY_ATTACK_DIST 0.5
#define ENEMY_RETREAT_DIST 5
#define ENEMY_AGRO_DIST 15
#define ENEMY_CHASE_DIST 25
#define ENEMY_ATTACK_COOLDOWN_TICKS 20

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

typedef struct {
    float x;
    float y;
} Posf;
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
    Posf vel;
} Entity;

typedef struct {
    Entity entity; // player's repr in the world
} NewPlayer;

typedef enum {
    StateIdle,
    StateChase,
    StateAttack,
    StateCooldown, // used for any cooldown
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
} Enemy;

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

float posf_distance(Posf from, Posf to) {
    Posf dist_vec = posf_subtract(from, to);

    return posf_magnitute(dist_vec);
}

Posf posf_set_magnitute(Posf pos, float mag) {
    float current_mag = posf_magnitute(pos);
    float scale_factor = current_mag / mag;
    return (Posf){
        .x = pos.x * scale_factor,
        .y = pos.y * scale_factor,
    };
}

Posf posf_direction(Posf from, Posf to) {
    Posf out = posf_subtract(from, to);
    float mag = posf_magnitute(out);

    return posf_set_magnitute(out, 1);
}

void entity_move(Entity *e, Map map) {
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
    float spawn_dist = posf_distance(e->entity.pos, e->spawn);
    if (spawn_dist < ENEMY_RETREAT_DIST) {
        e->state = StateIdle;
        return;
    }
    Posf spawn_direction = posf_direction(e->entity.pos, e->spawn);
    e->entity.vel = posf_set_magnitute(spawn_direction, ENEMY_VELOCITY);
}

void enemy_attack(Enemy *e, Entity *target) {
    // TODO: do attack
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
    Posf player_vec = posf_subtract(e->entity.pos, target->pos);
    float player_dist = posf_magnitute(player_vec);
    if (player_dist < ENEMY_ATTACK_DIST) {
        e->state = StateAttack;
        return;
    }
    Posf chase_vec = posf_set_magnitute(player_vec, ENEMY_VELOCITY);
    e->entity.vel = chase_vec;
}

void enemy_cooldown(Enemy *e, Entity *target) {
    e->cooldown_ticks -= 1;
    if (e->cooldown_ticks < 0) {
        e->state = e->cooldown_next_state;
    }
}

void enemy_retreat(Enemy *e, Entity *target) {
    Posf player_vec = posf_subtract(e->entity.pos, target->pos);
    float player_dist = posf_magnitute(player_vec);
    if (player_dist > ENEMY_RETREAT_DIST) {
        // TODO: sometimes strafe
        e->state = StateChase;
        return;
    }
    Posf retreat_vec = posf_reverse(player_vec);
    e->entity.vel = posf_set_magnitute(retreat_vec, ENEMY_VELOCITY);
}

void enemy_strafe(Enemy *e, Entity *target) {}

void enemy_update(Enemy *e, Entity *target) {
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
}