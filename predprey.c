#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <raylib.h>

#include "game.h"
#include "util.h"

enum ui_mode {
    UI_MODE_ADD,
    UI_MODE_DEL
};

enum ui_colorscheme {
    UI_COLORS_DEFAULT,
    UI_COLORS_DARK_RED,
    UI_COLORS_DARK_PINK,
    UI_COLORS_COUNT
};

struct ui {
    int sidebar_w;
    int sidebar_margin;
    int sidebar_line_h;

    int target_fps;
    enum ui_mode sel_mode;
    enum ui_colorscheme sel_colors;
    int sel_setting;

    int simulating;

    Color col_pred;
    Color col_prey;
    Color col_field;
    Color col_sidebar;
    Color col_sidebar_text;
    Color col_sidebar_text_sel;
};
static struct ui ui;

void ui_colors(enum ui_colorscheme idx)
{
    switch (idx) {
        case UI_COLORS_DARK_RED:
            ui.col_pred = RED;
            ui.col_prey = WHITE;
            ui.col_field = (Color) { 20,20,20,255 };
            ui.col_sidebar = DARKGRAY;
            ui.col_sidebar_text = WHITE;
            ui.col_sidebar_text_sel = RED;
            break;
        case UI_COLORS_DARK_PINK:
            ui.col_pred = (Color) { 146,92,255,255 };
            ui.col_prey = (Color) { 0,255,217,255 };
            ui.col_field = (Color) { 20,20,20,255 };
            ui.col_sidebar = DARKGRAY;
            ui.col_sidebar_text = WHITE;
            ui.col_sidebar_text_sel = ui.col_pred;
            break;
        default:
            ui.col_pred = RED;
            ui.col_prey = GREEN;
            ui.col_field = WHITE;
            ui.col_sidebar = LIGHTGRAY;
            ui.col_sidebar_text = BLACK;
            ui.col_sidebar_text_sel = MAROON;
    }
}

static void setting_int(int idx, Rectangle *bounds, const char *label,
        int *val, int min_val, int max_val)
{
    Color col = idx == ui.sel_setting
        ? ui.col_sidebar_text_sel : ui.col_sidebar_text;

    /* value (right aligned) */
    char num[32];
    snprintf(num, sizeof(num), "%d", *val);
    int vw = MeasureText(num, bounds->height);
    DrawText(num, bounds->x + bounds->width - vw, bounds->y,
            bounds->height, col);

    /* label (left aligned) */
    DrawText(label, bounds->x, bounds->y, bounds->height, col);

    bounds->y += bounds->height;

    if (idx != ui.sel_setting)
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
    enum { num_settings = 14 };
    int i = 0;
    bounds.height = ui.sidebar_line_h;

    /* handle keyboard */
    if (IsKeyPressed(KEY_UP))
        ui.sel_setting--;
    else if (IsKeyPressed(KEY_DOWN))
        ui.sel_setting++;

    /* handle mouse */
    Vector2 mouse_delta = GetMouseDelta();
    if (mouse_delta.x != 0.0f || mouse_delta.y != 0.0f) {
        Vector2 mouse = GetMousePosition();
        if (mouse.x >= bounds.x)
            ui.sel_setting = (mouse.y - bounds.y) / bounds.height;
    }

    ui.sel_setting = max(0, min(ui.sel_setting, num_settings - 1));

    setting_int(i++, &bounds, "colors",
            (int *)&ui.sel_colors, 0, UI_COLORS_COUNT - 1);
    ui_colors(ui.sel_colors);

    setting_int(i++, &bounds, "target fps", &ui.target_fps, 0, 120);
    SetTargetFPS(ui.target_fps);

    setting_int(i++, &bounds, "tick rate", &g.tick_rate, 1, 120);
    setting_int(i++, &bounds, "width", &g.w, 5, 400);
    setting_int(i++, &bounds, "height", &g.h, 5, 400);
    game_resize();

    int wrap = g.neighbourhood == neighbourhood4wrap;
    setting_int(i++, &bounds, "wrap field", &wrap, 0, 1);
    g.neighbourhood = wrap ? neighbourhood4wrap : neighbourhood4;

    setting_int(i++, &bounds, "sat birth",
            &g.satiety_min_for_birth, 0, g.value_max);
    setting_int(i++, &bounds, "sat hunt",
            &g.satiety_max_for_hunting, 0, g.value_max);
    setting_int(i++, &bounds, "sat dec",
            &g.satiety_decrement, 0, g.value_max);
    setting_int(i++, &bounds, "sat birth dec",
            &g.satiety_penalty_for_birth, 0, g.value_max);

    /* convert to % and round to nearest */
    int sat_hp_rate = 0.5f + 100.0f * g.satiety_to_health_ratio;
    setting_int(i++, &bounds, "sat/hp rate %", &sat_hp_rate, 0, 1000);
    g.satiety_to_health_ratio = sat_hp_rate / 100.0f;

    setting_int(i++, &bounds, "hp birth",
            &g.health_min_for_birth, 0, g.value_max);
    setting_int(i++, &bounds, "hp inc", &g.health_increment, 0, g.value_max);
    setting_int(i++, &bounds, "hp birth dec",
            &g.health_penalty_for_birth, 0, g.value_max);

    assert(num_settings == i);
}

static void stat_int(Rectangle *bounds, const char *label, int val)
{
    /* value (right aligned) */
    char num[32];
    snprintf(num, sizeof(num), "%d", val);
    int vw = MeasureText(num, bounds->height);
    DrawText(num, bounds->x + bounds->width - vw, bounds->y,
            bounds->height, ui.col_sidebar_text);

    /* label (left aligned) */
    DrawText(label, bounds->x, bounds->y, bounds->height, ui.col_sidebar_text);

    bounds->y -= bounds->height;
}

static void handle_stats(Rectangle bounds)
{
    bounds.y += bounds.height;
    bounds.height = ui.sidebar_line_h;
    bounds.y -= bounds.height;

    stat_int(&bounds, "preys", g.stat.preys);
    stat_int(&bounds, "preds", g.stat.preds);
}

static void handle_sidebar(Rectangle bounds)
{
    DrawRectangleRec(bounds, ui.col_sidebar);

    float m = ui.sidebar_margin;
    Rectangle content = {
        bounds.x + m, bounds.y + m,
        bounds.width - 2 * m, bounds.height - 2 * m
    };

    handle_settings(content);
    handle_stats(content);
}

static void handle_field(Rectangle bounds)
{
    DrawRectangleRec(bounds, ui.col_field);

    int x, y;
    Vector2 tile = { bounds.width / g.w, bounds.height / g.h };
    for (x = 0; x < g.w; x++) {
        for (y = 0; y < g.h; y++) {
            struct cell c = g.c[y * g.w + x];

            if (c.type == CELL_EMPTY)
                continue;

            Color col = c.type == CELL_PRED ? ui.col_pred : ui.col_prey;
            int m = g.value_max;
            col = ColorBrightness(col, (c.value - m) / (float)(2 * m));
            DrawRectangle(
                    floorf(x * tile.x), floorf(y * tile.y),
                    ceilf(tile.x), ceilf(tile.y), col);
        }
    }
}

static void handle_add_del(Rectangle field)
{
    Vector2 mouse = GetMousePosition();
    if (CheckCollisionPointRec(mouse, field)) {
        int x = (mouse.x - field.x) / field.width * g.w;
        int y = (mouse.y - field.y) / field.height * g.h;

        int l = IsMouseButtonDown(MOUSE_BUTTON_LEFT);
        if (!l && !IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
            return;

        if (ui.sel_mode == UI_MODE_ADD)
            game_add(l ? CELL_PRED : CELL_PREY, x, y);
        else
            game_del(x, y);
    }
}

int ui_mainloop()
{
    float elapsed = 0.0f; /* time passed since last tick */

    while (!WindowShouldClose()) {
        int w = GetRenderWidth();
        int h = GetRenderHeight();
        Rectangle sidebar = { w - ui.sidebar_w, 0, ui.sidebar_w, h };
        Rectangle field = { 0, 0, sidebar.x, h };

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_P))
            ui.simulating = !ui.simulating;

        if (IsKeyPressed(KEY_DELETE))
            game_clear();

        if (IsKeyPressed(KEY_A))
            ui.sel_mode = UI_MODE_ADD;
        else if (IsKeyPressed(KEY_D))
            ui.sel_mode = UI_MODE_DEL;

        handle_add_del(field);

        if (ui.simulating) {
            elapsed += GetFrameTime();
            elapsed = game_simulate(elapsed);
        }

        BeginDrawing();
        handle_sidebar(sidebar);
        handle_field(field);
        EndDrawing();
    }
    return 0;
}

void ui_init_default()
{
    ui.target_fps = 30;
    ui.sel_mode = UI_MODE_ADD;
    ui.sel_setting = 0;
    ui.sel_colors = 0;

    int window_w = 900;
    int window_h = 600;
    ui.sidebar_w = 300;
    ui.sidebar_margin = 5;
    ui.sidebar_line_h = 30;

    ui.simulating = 0;

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(window_w, window_h, "Predator-prey");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetTargetFPS(ui.target_fps);
}

void ui_free()
{
    CloseWindow();
}

int main(int argc, char **argv)
{
    game_init_default();
    ui_init_default();

    int ret = ui_mainloop();

    ui_free();
    game_free();
    return ret;
}
