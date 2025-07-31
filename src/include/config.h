#ifndef CONFIG_H
#define CONFIG_H

#include <Carbon/Carbon.h>

typedef struct {
    char prefix_selected[16];
    char prefix_unselected[16];
    char prefix_favorite[16];
    char prefix_active[16];

    int font_size;

    int hotkey_modifier; // e.g., cmdKey
    int hotkey_keycode;  // e.g., kVK_Space

    char key_quit;
    char key_favorite;
    char key_up;
    char key_down;
    char key_activate;

    char key_reload;
} SeeWinConfig;

extern SeeWinConfig config;

void load_config(void);
void ensure_default_config(void);

#define PREFIX_SELECTED   (config.prefix_selected)
#define PREFIX_UNSELECTED (config.prefix_unselected)
#define PREFIX_FAVORITE   (config.prefix_favorite)
#define PREFIX_ACTIVE     (config.prefix_active)
#define FONT_SIZE         (config.font_size)
#define KEY_QUIT          (config.key_quit)
#define KEY_FAVORITE      (config.key_favorite)
#define KEY_UP            (config.key_up)
#define KEY_DOWN          (config.key_down)
#define KEY_ACTIVATE      (config.key_activate)
#define KEY_RELOAD        (config.key_reload)

#endif
