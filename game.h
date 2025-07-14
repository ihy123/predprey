#ifndef GAME_H_SENTRY
#define GAME_H_SENTRY

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

int neighbourhood4(int x, int y, int w, int h, int *idx_out);
int neighbourhood4wrap(int x, int y, int w, int h, int *idx_out);

/* main game struct */
struct game {
    /* the field */
    struct cell *c;
    /* maps cell to bool: true if a cell was already handled in cur step */
    unsigned char *handled;
    int w;
    int h;
    int cells_cap; /* capaticy of the <cells> and <handled> buffers */

    struct {
        int preys;
        int preds;
    } stat;

    /* settings */
    int tick_rate;
    int value_max;
    fn_neighbourhood neighbourhood;

    /* adjustable coefficients, for balance */
    int satiety_min_for_birth;
    int satiety_max_for_hunting;
    int satiety_decrement;
    int satiety_penalty_for_birth;
    float satiety_to_health_ratio;

    int health_min_for_birth;
    int health_increment;
    int health_penalty_for_birth;
};
extern struct game g;

void game_init_default();
/* elapsed is time since last tick; returns updated time since last tick */
float game_simulate(float elapsed);
void game_add(enum cell_type type, int x, int y);
void game_del(int x, int y);
void game_resize();
void game_clear();
void game_free();

#endif
