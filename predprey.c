#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

typedef int (*fn_neighbourhood)(int x, int y, int w, int h, int *idx_out);

struct game {
    /* the field */
    struct cell *c;
    /* maps cell to bool: true if a cell was already handled in cur step */
    unsigned char *handled;
    int w;
    int h;
    int cells_cap; /* capaticy of the <cells> and <handled> buffers */

    /* stats */
    int preys;
    int preds;
    int preys_avg;
    int preds_avg;
    int preys_std;
    int preds_std;
    int health_avg;
    int satiety_avg;
};
static struct game g;

struct settings {
    /* settings */
    int target_fps;
    int target_fps_prev;
    int tick_rate;

    /* adjustable coefficients, for balance */
    int value_max;
    fn_neighbourhood neighbourhood;

    int satiety_min_for_birth;
    int satiety_max_for_hunting;
    int satiety_decrement;
    int satiety_penalty_for_birth;
    float satiety_to_health_ratio;

    int health_min_for_birth;
    int health_increment;
    int health_penalty_for_birth;
};
static struct settings s;

static void prey_feed(struct cell *c)
{
    c->health = min(c->health + s.health_increment, s.value_max);
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
            if (n->health >= s.health_min_for_birth) {
                /* leave child in source cell */
                n->health -= s.health_penalty_for_birth;
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
            int count = s.neighbourhood(x, y, g.w, g.h, neighbours);

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
            c->satiety += s.satiety_to_health_ratio * n->health;
            n->satiety = c->satiety = min(c->satiety, s.value_max);

            if (n->satiety >= s.satiety_min_for_birth) {
                /* leave child in source cell */
                n->satiety -= s.satiety_penalty_for_birth;
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

            if (n->satiety >= s.satiety_min_for_birth) {
                /* leave child in source cell */
                n->satiety -= s.satiety_penalty_for_birth;
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
            int count = s.neighbourhood(x, y, g.w, g.h, neighbours);

            int moved = 0;
            if (c->satiety <= s.satiety_max_for_hunting)
                moved = pred_hunt(c, neighbours, count);

            if (!moved) {
                c->satiety -= s.satiety_decrement;
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

static float simulate(float elapsed)
{
    float tick_dur = 1.0f / s.tick_rate;
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

static void game_resize()
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

static void game_clean()
{
    memset(g.c, 0, g.cells_cap * sizeof(*g.c));
    memset(g.handled, 0, g.cells_cap * sizeof(*g.handled));
}

enum field_mode {
    MODE_ADD,
    MODE_DEL
};

static const Color col_pred = RED;
static const Color col_prey = GREEN;
static const Color col_field = WHITE;
static const Color col_sidebar = LIGHTGRAY;
static const Color col_sidebar_text = BLACK;
static const Color col_sidebar_text_sel = MAROON;

static int sel_setting = 0;

static void setting_int(int idx, Rectangle bounds, const char *label,
        int *val, int min_val, int max_val)
{
    Color col = idx == sel_setting ? col_sidebar_text_sel : col_sidebar_text;

    /* value */
    char num[32];
    snprintf(num, sizeof(num), "%d", *val);
    int vw = MeasureText(num, bounds.height);
    DrawText(num, bounds.x + bounds.width - vw, bounds.y, bounds.height, col);

    /* label */
    int lw = MeasureText(label, bounds.height);
    DrawText(label, bounds.x + bounds.width - vw - lw - bounds.height / 2,
            bounds.y, bounds.height, col);

    if (idx != sel_setting)
        return;

    /* handle input */
    if (IsKeyDown(KEY_LEFT))
        (*val)--;
    else if (IsKeyDown(KEY_RIGHT))
        (*val)++;

    *val += GetMouseWheelMove();

    /* clamp input */
    *val = max(min_val, min(*val, max_val));
}

static void handle_settings(Rectangle bounds)
{
    static int num_settings = 0;

    if (IsKeyPressed(KEY_UP))
        sel_setting--;
    else if (IsKeyPressed(KEY_DOWN))
        sel_setting++;
    sel_setting = max(0, min(sel_setting, num_settings - 1));

    Vector2 mouse_delta = GetMouseDelta();
    if (mouse_delta.x > 0.0f || mouse_delta.y > 0.0f) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, bounds))
            sel_setting = (mouse.y - bounds.y) / 30;
    }

    num_settings = 0;
    bounds.height = 30;

    setting_int(num_settings++, bounds, "Tick rate", &s.tick_rate, 1, 120);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "Target FPS", &s.target_fps, 0, 120);
    bounds.y += bounds.height;

    setting_int(num_settings++, bounds, "Width", &g.w, 5, 400);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "Height", &g.h, 5, 400);
    bounds.y += bounds.height;

    int wrap = s.neighbourhood == neighbourhood4wrap;
    setting_int(num_settings++, bounds, "Wrap field", &wrap, 0, 1);
    bounds.y += bounds.height;
    s.neighbourhood = wrap ? neighbourhood4wrap : neighbourhood4;

    setting_int(num_settings++, bounds, "sat birth", &s.satiety_min_for_birth, 0, s.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "sat hunt", &s.satiety_max_for_hunting, 0, s.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "sat dec", &s.satiety_decrement, 0, s.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "sat birth dec", &s.satiety_penalty_for_birth, 0, s.value_max);
    bounds.y += bounds.height;

    /* convert to % and round to nearest */
    int sat_hp_rate = 0.5f + 100.0f * s.satiety_to_health_ratio;
    setting_int(num_settings++, bounds, "sat/hp rate %", &sat_hp_rate, 0, 1000);
    bounds.y += bounds.height;
    s.satiety_to_health_ratio = sat_hp_rate / 100.0f;

    setting_int(num_settings++, bounds, "hp birth", &s.health_min_for_birth, 0, s.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "hp inc", &s.health_increment, 0, s.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "hp birth dec", &s.health_penalty_for_birth, 0, s.value_max);
    bounds.y += bounds.height;
}

int main(int argc, char **argv)
{
    s.target_fps = 30;
    s.tick_rate = 30;

    s.value_max = 100;

    s.neighbourhood = neighbourhood4;
    s.satiety_min_for_birth = 60;
    s.satiety_max_for_hunting = 70;
    s.satiety_decrement = 7;
    s.satiety_penalty_for_birth = 30;
    s.satiety_to_health_ratio = 0.2f;

    s.health_min_for_birth = 70;
    s.health_increment = 10;
    s.health_penalty_for_birth = 50;

    memset(&g, 0, sizeof(g));
    g.w = 100;
    g.h = 100;
    int w_prev = g.w;
    int h_prev = g.h;
    game_resize();

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(800, 500, "Predator-prey");
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(s.target_fps);
    s.target_fps_prev = s.target_fps;

    /* TODO: don't render to a texture, but draw the field directly */
    RenderTexture2D field_texture = LoadRenderTexture(g.w, g.h);

    enum field_mode sel_mode = MODE_ADD;

    int simulating = 0;
    float elapsed = 0.0f; /* time passed since last tick */
    while (!WindowShouldClose()) {
        /* screen size */
        int sw = GetRenderWidth();
        int sh = GetRenderHeight();
        /* sidebar rectangle */
        int sbw = 300;
        int sbh = sh;
        int sbx = sw - sbw;
        int sby = 0;

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_P))
            simulating = !simulating;

        if (IsKeyPressed(KEY_DELETE))
            game_clean();

        if (IsKeyPressed(KEY_A))
            sel_mode = MODE_ADD;
        else if (IsKeyPressed(KEY_D))
            sel_mode = MODE_DEL;

        Rectangle rect = { 0, 0, sbx, sh };
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, rect)) {
            int x = (mouse.x - rect.x) / rect.width * g.w;
            int y = (mouse.y - rect.y) / rect.height * g.h;
            struct cell *c = &g.c[y * g.w + x];

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if (sel_mode == MODE_ADD) {
                    c->type = CELL_PRED;
                    c->satiety = s.value_max;
                } else {
                    c->type = CELL_EMPTY;
                }
            } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                if (sel_mode == MODE_ADD) {
                    c->type = CELL_PREY;
                    c->health = s.value_max;
                } else {
                    c->type = CELL_EMPTY;
                }
            }
        }

        if (simulating) {
            elapsed += GetFrameTime();
            elapsed = simulate(elapsed);
        }

        BeginDrawing();

        /* menu sidebar */
        rect = (Rectangle) { sbx + 5, sby + 5, sbw - 10, sbh - 10 };
        DrawRectangle(sbx, sby, sbw, sbh, col_sidebar);

        handle_settings(rect);

        if (s.target_fps != s.target_fps_prev) {
            s.target_fps_prev = s.target_fps;
            SetTargetFPS(s.target_fps);
        }

        if (g.w != w_prev || g.h != h_prev) {
            w_prev = g.w;
            h_prev = g.h;
            field_texture = LoadRenderTexture(g.w, g.h);
            game_resize();
        }

        /* field */
        BeginTextureMode(field_texture);
        ClearBackground(col_field);

        int x, y;
        for (x = 0; x < g.w; x++) {
            for (y = 0; y < g.h; y++) {
                struct cell c = g.c[y * g.w + x];

                if (c.type == CELL_EMPTY)
                    continue;

                Color col = c.type == CELL_PRED ? col_pred : col_prey;
                int m = s.value_max;
                col = ColorBrightness(col, (c.value - m) / (float)(2 * m));
                DrawRectangle(x, y, 1, 1, col);
            }
        }
        EndTextureMode();

        Rectangle src = { 0, 0, g.w, -g.h };
        Rectangle dst = { 0, 0, sbx, sh };
        Vector2 origin = { 0, 0 };
        DrawTexturePro(field_texture.texture, src, dst, origin, 0.0f, WHITE);

        EndDrawing();
    }

    UnloadRenderTexture(field_texture);
    CloseWindow();
    return 0;
}
