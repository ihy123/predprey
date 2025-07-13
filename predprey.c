#include <stdio.h>
#include <string.h>

#include <raylib.h>

#include "game.h"
#include "util.h"

enum field_mode {
    MODE_ADD,
    MODE_DEL
};

struct ui {
    int sidebar_w;
    int sidebar_margin;

    int target_fps;
    int target_fps_prev;
    int w_prev;
    int h_prev;
    enum field_mode sel_mode;
    RenderTexture2D field_texture;

    Color col_pred;
    Color col_prey;
    Color col_field;
    Color col_sidebar;
    Color col_sidebar_text;
    Color col_sidebar_text_sel;
};
static struct ui ui;

static int sel_setting = 0;

static void setting_int(int idx, Rectangle bounds, const char *label,
        int *val, int min_val, int max_val)
{
    Color col = idx == sel_setting ? ui.col_sidebar_text_sel : ui.col_sidebar_text;

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

    setting_int(num_settings++, bounds, "Tick rate", &g.tick_rate, 1, 120);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "Target FPS", &ui.target_fps, 0, 120);
    bounds.y += bounds.height;

    setting_int(num_settings++, bounds, "Width", &g.w, 5, 400);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "Height", &g.h, 5, 400);
    bounds.y += bounds.height;

    int wrap = g.neighbourhood == neighbourhood4wrap;
    setting_int(num_settings++, bounds, "Wrap field", &wrap, 0, 1);
    bounds.y += bounds.height;
    g.neighbourhood = wrap ? neighbourhood4wrap : neighbourhood4;

    setting_int(num_settings++, bounds, "sat birth", &g.satiety_min_for_birth, 0, g.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "sat hunt", &g.satiety_max_for_hunting, 0, g.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "sat dec", &g.satiety_decrement, 0, g.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "sat birth dec", &g.satiety_penalty_for_birth, 0, g.value_max);
    bounds.y += bounds.height;

    /* convert to % and round to nearest */
    int sat_hp_rate = 0.5f + 100.0f * g.satiety_to_health_ratio;
    setting_int(num_settings++, bounds, "sat/hp rate %", &sat_hp_rate, 0, 1000);
    bounds.y += bounds.height;
    g.satiety_to_health_ratio = sat_hp_rate / 100.0f;

    setting_int(num_settings++, bounds, "hp birth", &g.health_min_for_birth, 0, g.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "hp inc", &g.health_increment, 0, g.value_max);
    bounds.y += bounds.height;
    setting_int(num_settings++, bounds, "hp birth dec", &g.health_penalty_for_birth, 0, g.value_max);
    bounds.y += bounds.height;
}

enum ui_colorscheme {
    UI_COLORS_DEFAULT,
    UI_COLORS_COUNT
};

void ui_colors(enum ui_colorscheme idx)
{
    ui.col_pred = RED;
    ui.col_prey = GREEN;
    ui.col_field = WHITE;
    ui.col_sidebar = LIGHTGRAY;
    ui.col_sidebar_text = BLACK;
    ui.col_sidebar_text_sel = MAROON;
}

void ui_init_default()
{
    ui.target_fps = 30;
    ui.w_prev = g.w;
    ui.h_prev = g.h;
    ui.sel_mode = MODE_ADD;

    SetTraceLogLevel(LOG_ERROR);
    InitWindow(900, 600, "Predator-prey");
    ui.sidebar_w = 300;
    ui.sidebar_margin = 5;
    SetWindowState(FLAG_WINDOW_RESIZABLE);

    SetTargetFPS(ui.target_fps);
    ui.target_fps_prev = ui.target_fps;

    /* TODO: don't render to a texture, but draw the field directly */
    ui.field_texture = LoadRenderTexture(g.w, g.h);

    ui_colors(UI_COLORS_DEFAULT);
}

void ui_free()
{
    UnloadRenderTexture(ui.field_texture);
    CloseWindow();
}

int ui_mainloop()
{
    int simulating = 0;
    float elapsed = 0.0f; /* time passed since last tick */

    while (!WindowShouldClose()) {
        /* screen size */
        int sw = GetRenderWidth();
        int sh = GetRenderHeight();
        /* sidebar size */
        Rectangle sb = { sw - ui.sidebar_w, 0, ui.sidebar_w, sh };
        Rectangle field = { 0, 0, sb.x, sh };

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_P))
            simulating = !simulating;

        if (IsKeyPressed(KEY_DELETE))
            game_clean();

        if (IsKeyPressed(KEY_A))
            ui.sel_mode = MODE_ADD;
        else if (IsKeyPressed(KEY_D))
            ui.sel_mode = MODE_DEL;

        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, field)) {
            int x = (mouse.x - field.x) / field.width * g.w;
            int y = (mouse.y - field.y) / field.height * g.h;
            struct cell *c = &g.c[y * g.w + x];

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                if (ui.sel_mode == MODE_ADD) {
                    c->type = CELL_PRED;
                    c->satiety = g.value_max;
                } else {
                    c->type = CELL_EMPTY;
                }
            } else if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                if (ui.sel_mode == MODE_ADD) {
                    c->type = CELL_PREY;
                    c->health = g.value_max;
                } else {
                    c->type = CELL_EMPTY;
                }
            }
        }

        if (simulating) {
            elapsed += GetFrameTime();
            elapsed = game_simulate(elapsed);
        }

        BeginDrawing();

        /* menu sidebar */
        DrawRectangleRec(sb, ui.col_sidebar);
        float m = ui.sidebar_margin;
        Rectangle sb_content = {
            sb.x + m, sb.y + m,
            sb.width - 2 * m, sb.height - 2 * m
        };

        handle_settings(sb_content);

        if (ui.target_fps != ui.target_fps_prev) {
            ui.target_fps_prev = ui.target_fps;
            SetTargetFPS(ui.target_fps);
        }

        if (g.w != ui.w_prev || g.h != ui.h_prev) {
            ui.w_prev = g.w;
            ui.h_prev = g.h;
            ui.field_texture = LoadRenderTexture(g.w, g.h);
            game_resize();
        }

        /* field */
        BeginTextureMode(ui.field_texture);
        ClearBackground(ui.col_field);

        int x, y;
        for (x = 0; x < g.w; x++) {
            for (y = 0; y < g.h; y++) {
                struct cell c = g.c[y * g.w + x];

                if (c.type == CELL_EMPTY)
                    continue;

                Color col = c.type == CELL_PRED ? ui.col_pred : ui.col_prey;
                int m = g.value_max;
                col = ColorBrightness(col, (c.value - m) / (float)(2 * m));
                DrawRectangle(x, y, 1, 1, col);
            }
        }
        EndTextureMode();

        Rectangle src = { 0, 0, g.w, -g.h };
        Rectangle dst = { 0, 0, sb.x, sh };
        Vector2 origin = { 0, 0 };
        DrawTexturePro(ui.field_texture.texture, src, dst, origin, 0.0f, WHITE);

        EndDrawing();
    }
    return 0;
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
