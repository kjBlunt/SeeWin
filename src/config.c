#include "include/config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

SeeWinConfig config = {
    .prefix_selected = "\u25B6 ",
    .prefix_unselected = "  ",
    .prefix_favorite = "\u2605 ",
    .prefix_active = "* ",
    .font_size = 15,
    .hotkey_modifier = cmdKey,
    .hotkey_keycode = kVK_Space,
    .key_quit = 'q',
    .key_favorite = 'f',
    .key_up = 'j',
    .key_down = 'k',
    .key_activate = '\r'
};

static int keycode_from_name(const char* name) {
    if (strcmp(name, "space") == 0) return kVK_Space;
    if (strcmp(name, "a") == 0) return kVK_ANSI_A;
    if (strcmp(name, "b") == 0) return kVK_ANSI_B;
    if (strcmp(name, "j") == 0) return kVK_ANSI_J;
    if (strcmp(name, "k") == 0) return kVK_ANSI_K;
    if (strcmp(name, "return") == 0) return kVK_Return;
    return -1;
}

void ensure_default_config(void) {
    char path[512];
    snprintf(path, sizeof(path), "%s/.seewinrc", getenv("HOME"));

    struct stat buffer;
    if (stat(path, &buffer) == 0) return;

    FILE* f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "# Default SeeWin config\n");
    fprintf(f, "prefix_selected=\u25B6 \n");
    fprintf(f, "prefix_unselected=  \n");
    fprintf(f, "prefix_favorite=\u2605 \n");
    fprintf(f, "prefix_active=* \n");
    fprintf(f, "font_size=15\n");
    fprintf(f, "hotkey_modifier=cmd\n");
    fprintf(f, "hotkey_key=space\n");
    fprintf(f, "key_quit=q\n");
    fprintf(f, "key_favorite=f\n");
    fprintf(f, "key_up=j\n");
    fprintf(f, "key_down=k\n");
    fprintf(f, "key_activate=\r\n");

    fclose(f);
}

void load_config() {
    ensure_default_config();

    char path[512];
    snprintf(path, sizeof(path), "%s/.seewinrc", getenv("HOME"));
    FILE* file = fopen(path, "r");
    if (!file) return;

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char* eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char* key = line;
        char* val = eq + 1;

        // Strip newline only
        val[strcspn(val, "\r\n")] = '\0';

        // Optional quote handling
        if (val[0] == '\"') {
            size_t len = strlen(val);
            if (len > 1 && val[len - 1] == '\"') {
                val[len - 1] = '\0';
                val++;
            }
        }

        if (strcmp(key, "prefix_selected") == 0) strncpy(config.prefix_selected, val, sizeof(config.prefix_selected) - 1);
        else if (strcmp(key, "prefix_unselected") == 0) strncpy(config.prefix_unselected, val, sizeof(config.prefix_unselected) - 1);
        else if (strcmp(key, "prefix_favorite") == 0) strncpy(config.prefix_favorite, val, sizeof(config.prefix_favorite) - 1);
        else if (strcmp(key, "prefix_active") == 0) strncpy(config.prefix_active, val, sizeof(config.prefix_active) - 1);
        else if (strcmp(key, "font_size") == 0) config.font_size = atoi(val);
        else if (strcmp(key, "hotkey_modifier") == 0) {
            if (strcmp(val, "cmd") == 0) config.hotkey_modifier = cmdKey;
            else if (strcmp(val, "option") == 0) config.hotkey_modifier = optionKey;
            else if (strcmp(val, "control") == 0) config.hotkey_modifier = controlKey;
        }
        else if (strcmp(key, "hotkey_key") == 0) {
            int code = keycode_from_name(val);
            if (code >= 0) config.hotkey_keycode = code;
        }
        else if (strcmp(key, "key_quit") == 0) config.key_quit = val[0];
        else if (strcmp(key, "key_favorite") == 0) config.key_favorite = val[0];
        else if (strcmp(key, "key_up") == 0) config.key_up = val[0];
        else if (strcmp(key, "key_down") == 0) config.key_down = val[0];
        else if (strcmp(key, "key_activate") == 0) config.key_activate = val[0];
        else if (strcmp(key, "key_reload") == 0) config.key_reload = val[0];
    }

    fclose(file);
}

