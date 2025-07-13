#include <stdlib.h>
#include <string.h>

#include "game.h"
#include "util.h"

int neighbourhood4(int x, int y, int w, int h, int *idx_out)
{
    int count = 0;
    if (x > 0) {
        idx_out[count] = y * w + x - 1;
        count++;
    }
    if (y > 0) {
        idx_out[count] = (y - 1) * w + x;
        count++;
    }
    if (x + 1 < w) {
        idx_out[count] = y * w + x + 1;
        count++;
    }
    if (y + 1 < h) {
        idx_out[count] = (y + 1) * w + x;
        count++;
    }
    rand_shuffle(idx_out, count);
    return count;
}

int neighbourhood4wrap(int x, int y, int w, int h, int *idx_out)
{
    idx_out[0] = y * w + (x > 0 ? x : w) - 1;
    idx_out[1] = ((y > 0 ? y : h) - 1) * w + x;
    idx_out[2] = y * w + (x + 1 == w ? 0 : x + 1);
    idx_out[3] = (y + 1 == h ? 0 : y + 1) * w + x;

    rand_shuffle(idx_out, 4);
    return 4;
}

struct game g;

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
                n->health -= g.health_penalty_for_birth;
                if (n->health <= 0)
                    n->type = CELL_EMPTY;
                /* don't clean up source cell, leave child there */
            } else {
                /* clean up source cell */
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
            int count = g.neighbourhood(x, y, g.w, g.h, neighbours);

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
                n->satiety -= g.satiety_penalty_for_birth;
                if (n->satiety <= 0)
                    n->type = CELL_EMPTY;
                /* don't clean up source cell, leave child there */
            } else {
                /* clean up source cell */
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
                n->satiety -= g.satiety_penalty_for_birth;
                if (n->satiety <= 0)
                    n->type = CELL_EMPTY;
                /* don't clean up source cell, leave child there */
            } else {
                /* clean up source cell */
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
            int count = g.neighbourhood(x, y, g.w, g.h, neighbours);

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

static void calc_stats()
{
    g.preds = g.preys = 0;

    int i;
    for (i = 0; i < g.w * g.h; i++) {
        struct cell c = g.c[i];
        if (c.type == CELL_PRED) {
            g.preds++;
        } else if (c.type == CELL_PREY) {
            g.preys++;
        }
    }
}

void game_init_default()
{
    memset(&g, 0, sizeof(g));

    g.w = 100;
    g.h = 100;

    g.tick_rate = 30;
    g.value_max = 100;
    g.neighbourhood = neighbourhood4;

    g.satiety_min_for_birth = 60;
    g.satiety_max_for_hunting = 70;
    g.satiety_decrement = 7;
    g.satiety_penalty_for_birth = 30;
    g.satiety_to_health_ratio = 0.2f;

    g.health_min_for_birth = 70;
    g.health_increment = 10;
    g.health_penalty_for_birth = 50;

    game_resize();
}

float game_simulate(float elapsed)
{
    float tick_dur = 1.0f / g.tick_rate;
    int recalc_stats = elapsed >= tick_dur;

    while (elapsed >= tick_dur) {
        elapsed -= tick_dur;

        memset(g.handled, 0, g.w * g.h);
        preys_step();
        preds_step();
    }

    if (recalc_stats)
        calc_stats();

    return elapsed;
}

void game_resize()
{
    int size = g.w * g.h;
    if (g.cells_cap < size) {
        free(g.c);
        free(g.handled);

        g.cells_cap = max(size, 1.5f * g.cells_cap);
        g.c = calloc(g.cells_cap, sizeof(*g.c));
        g.handled = calloc(g.cells_cap, sizeof(*g.handled));
    }
}

void game_clean()
{
    memset(g.c, 0, g.cells_cap * sizeof(*g.c));
    memset(g.handled, 0, g.cells_cap * sizeof(*g.handled));
}

void game_free()
{
    free(g.c);
    free(g.handled);
}
