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
    if (strlen(name) == 1 && name[0] >= 'a' && name[0] <= 'z')
      return kVK_ANSI_A + (name[0] - 'a');

    if (strcmp(name, "0") == 0) return kVK_ANSI_0;
    if (strcmp(name, "1") == 0) return kVK_ANSI_1;
    if (strcmp(name, "2") == 0) return kVK_ANSI_2;
    if (strcmp(name, "3") == 0) return kVK_ANSI_3;
    if (strcmp(name, "4") == 0) return kVK_ANSI_4;
    if (strcmp(name, "5") == 0) return kVK_ANSI_5;
    if (strcmp(name, "6") == 0) return kVK_ANSI_6;
    if (strcmp(name, "7") == 0) return kVK_ANSI_7;
    if (strcmp(name, "8") == 0) return kVK_ANSI_8;
    if (strcmp(name, "9") == 0) return kVK_ANSI_9;

    if (strcmp(name, "return") == 0 || strcmp(name, "enter") == 0) return kVK_Return;
    if (strcmp(name, "tab") == 0) return kVK_Tab;
    if (strcmp(name, "space") == 0) return kVK_Space;
    if (strcmp(name, "delete") == 0) return kVK_Delete;
    if (strcmp(name, "escape") == 0 || strcmp(name, "esc") == 0) return kVK_Escape;
    if (strcmp(name, "left") == 0) return kVK_LeftArrow;
    if (strcmp(name, "right") == 0) return kVK_RightArrow;
    if (strcmp(name, "up") == 0) return kVK_UpArrow;
    if (strcmp(name, "down") == 0) return kVK_DownArrow;

    if (strcmp(name, "minus") == 0) return kVK_ANSI_Minus;
    if (strcmp(name, "equal") == 0) return kVK_ANSI_Equal;
    if (strcmp(name, "left_bracket") == 0) return kVK_ANSI_LeftBracket;
    if (strcmp(name, "right_bracket") == 0) return kVK_ANSI_RightBracket;
    if (strcmp(name, "backslash") == 0) return kVK_ANSI_Backslash;
    if (strcmp(name, "semicolon") == 0) return kVK_ANSI_Semicolon;
    if (strcmp(name, "quote") == 0) return kVK_ANSI_Quote;
    if (strcmp(name, "comma") == 0) return kVK_ANSI_Comma;
    if (strcmp(name, "period") == 0 || strcmp(name, "dot") == 0) return kVK_ANSI_Period;
    if (strcmp(name, "slash") == 0 || strcmp(name, "forwardslash") == 0) return kVK_ANSI_Slash;
    if (strcmp(name, "grave") == 0 || strcmp(name, "backtick") == 0) return kVK_ANSI_Grave;

    if (strncmp(name, "f", 1) == 0 && strlen(name) <= 3) {
        int fnum = atoi(name + 1);
        if (fnum >= 1 && fnum <= 19)
            return kVK_F1 + (fnum - 1);
    }

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
        else if (strcmp(key, "key_activate") == 0) {
          if (strcmp(val, "\\r") == 0) config.key_activate = '\r';
          else if (strcmp(val, "\\n") == 0) config.key_activate = '\n';
          else config.key_activate = val[0];
        }
        else if (strcmp(key, "key_reload") == 0) config.key_reload = val[0];
    }

    fclose(file);
}

