#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <raylib.h>

#include "util.h"

enum cell_type {
    CELL_EMPTY = 0,
    CELL_PRED,
    CELL_PREY
};

struct cell {
    enum cell_type type;
    union {
        int satiety;
        int health;
        int value;
    };
};

enum state {
    STATE_MENU,
    STATE_RUNNING,
    STATE_PAUSED
};

typedef int (*fn_neighbourhood)(int x, int y, int w, int h, int *idx_out);

struct game {
    /* the field */
    struct cell *c;
    /* maps cell to bool: true if a cell was already handled in cur step */
    unsigned char *handled;
    int w;
    int h;

    /* stats */
    int preys;
    int preds;
    int preys_avg;
    int preds_avg;
    int preys_std;
    int preds_std;
    int health_avg;
    int satiety_avg;

    /* adjustable coefficients */
    int value_max;

    fn_neighbourhood pred_neighbourhood;
    int satiety_min_for_birth;
    int satiety_max_for_hunting;
    int satiety_decrement;
    int satiety_penalty_for_birth;
    float satiety_to_health_ratio;

    fn_neighbourhood prey_neighbourhood;
    int health_min_for_birth;
    int health_increment;
    int health_penalty_for_birth;
};
static struct game g;

static void prey_feed(struct cell *c)
{
    c->health = min(c->health + g.health_increment, g.value_max);
}

static int prey_move(struct cell *c, int *neighbours, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        struct cell *n = &g.c[neighbours[i]];
        if (n->type == CELL_EMPTY) {
            /* move to destination cell */
            n->type = CELL_PREY;
            n->health = c->health;
            if (n->health >= g.health_min_for_birth) {
                /* leave child in source cell */
                n->health -= g.health_penalty_for_birth;
                if (n->health <= 0)
                    n->type = CELL_EMPTY;
            } else {
                c->type = CELL_EMPTY;
            }
            g.handled[neighbours[i]] = 1;
            return 1;
        }
    }
    return 0;
}

static void preys_step()
{
    int x, y;
    for (x = 0; x < g.w; x++) {
        for (y = 0; y < g.h; y++) {
            int ci = y * g.w + x;
            if (g.handled[ci])
                continue;

            struct cell *c = &g.c[ci];
            if (c->type != CELL_PREY)
                continue;

            prey_feed(c);

            int neighbours[8];
            int count = g.prey_neighbourhood(x, y, g.w, g.h, neighbours);

            int moved = prey_move(c, neighbours, count);
            if (!moved)
                c->type = CELL_EMPTY;

            g.handled[ci] = 1;
        }
    }
}

static int pred_hunt(struct cell *c, int *neighbours, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        struct cell *n = &g.c[neighbours[i]];
        if (n->type == CELL_PREY) {
            /* move to destination cell, consume prey */
            n->type = CELL_PRED;
            c->satiety += g.satiety_to_health_ratio * n->health;
            n->satiety = c->satiety = min(c->satiety, g.value_max);

            if (n->satiety >= g.satiety_min_for_birth) {
                /* leave child in source cell */
                n->satiety -= g.satiety_penalty_for_birth;
                if (n->satiety <= 0)
                    n->type = CELL_EMPTY;
            } else {
                c->type = CELL_EMPTY;
            }

            g.handled[neighbours[i]] = 1;
            return 1;
        }
    }
    return 0;
}

static int pred_move(struct cell *c, int *neighbours, int count)
{
    int i;
    for (i = 0; i < count; i++) {
        struct cell *n = &g.c[neighbours[i]];
        if (n->type == CELL_EMPTY) {
            /* move to destination cell */
            n->type = CELL_PRED;
            n->satiety = c->satiety;

            if (n->satiety >= g.satiety_min_for_birth) {
                /* leave child in source cell */
                n->satiety -= g.satiety_penalty_for_birth;
                if (n->satiety <= 0)
                    n->type = CELL_EMPTY;
            } else {
                c->type = CELL_EMPTY;
            }

            g.handled[neighbours[i]] = 1;
            return 1;
        }
    }
    return 0;
}

static void preds_step()
{
    int x, y;
    for (x = 0; x < g.w; x++) {
        for (y = 0; y < g.h; y++) {
            int ci = y * g.w + x;
            if (g.handled[ci])
                continue;

            struct cell *c = &g.c[ci];
            if (c->type != CELL_PRED)
                continue;

            int neighbours[8];
            int count = g.pred_neighbourhood(x, y, g.w, g.h, neighbours);

            int moved = 0;
            if (c->satiety <= g.satiety_max_for_hunting)
                moved = pred_hunt(c, neighbours, count);

            if (!moved) {
                c->satiety -= g.satiety_decrement;
                if (c->satiety > 0)
                    moved = pred_move(c, neighbours, count);
            }

            if (!moved)
                c->type = CELL_EMPTY;

            g.handled[ci] = 1;
        }
    }
}

int main(int argc, char **argv)
{
    enum state state = STATE_MENU;
    g.c = NULL;
    g.handled = NULL;
    g.w = g.h = 0;

    g.value_max = 100;

    g.pred_neighbourhood = neighbourhood4;
    g.satiety_min_for_birth = 60;
    g.satiety_max_for_hunting = 70;
    g.satiety_decrement = 7;
    g.satiety_penalty_for_birth = 30;
    g.satiety_to_health_ratio = 0.2f;

    g.prey_neighbourhood = neighbourhood4;
    g.health_min_for_birth = 70;
    g.health_increment = 10;
    g.health_penalty_for_birth = 50;

    InitWindow(800, 800, "Predator-prey");
    SetTargetFPS(30);

    while (!WindowShouldClose()) {
        switch (state) {
            case STATE_MENU:
                //if (IsKeyPressed(KEY_SPACE)) {
                    state = STATE_RUNNING;
                    g.w = g.h = 60;
                    g.c = calloc(g.w * g.h, sizeof(*g.c));
                    g.handled = calloc(g.w * g.h, 1);

                    g.c[0 * g.w + 5].type = CELL_PRED;
                    g.c[0 * g.w + 5].satiety = g.value_max;
                    g.c[5 * g.w + 5].type = CELL_PREY;
                    g.c[5 * g.w + 5].health = g.value_max;
                //}
                break;
            case STATE_RUNNING:
                if (IsKeyPressed(KEY_SPACE)) {
                    state = STATE_PAUSED;
                    break;
                }
                memset(g.handled, 0, g.w * g.h);
                preys_step();
                preds_step();
                break;
            case STATE_PAUSED:
                if (IsKeyPressed(KEY_SPACE))
                    state = STATE_RUNNING;
                break;
        }

        BeginDrawing();
        do {
            ClearBackground(RAYWHITE);

            if (state != STATE_RUNNING)
                break;

            int sw = GetScreenWidth();
            int sh = GetScreenHeight();
            int cw = sw / g.w;
            int ch = sh / g.h;

            int x, y;
            for (x = 0; x < g.w; x++) {
                for (y = 0; y < g.h; y++) {
                    struct cell c = g.c[y * g.w + x];

                    if (c.type == CELL_EMPTY)
                        continue;

                    Color col = c.type == CELL_PRED ? RED : GREEN;
                    int m = g.value_max;
                    col = ColorBrightness(col, (c.value - m) / (float)(2 * m));
                    DrawRectangle(x * cw, y * ch, cw, ch, col);
                }
            }
        } while (0);
        EndDrawing();
    }

    CloseWindow();
    free(g.c);
    free(g.handled);
    return 0;
}
