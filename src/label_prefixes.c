#include "include/label_prefixes.h"
#include <string.h>
#include <stdlib.h>

const char* PREFIX_SELECTED   = "\u25B6 ";
const char* PREFIX_UNSELECTED = "  ";
const char* PREFIX_FAVORITE   = "★ ";
const char* PREFIX_ACTIVE     = "* ";

int get_prefix_bit_length(const char* prefix) {
    return (int)strlen(prefix);
}

const char* strip_prefix(const char* str) {
    if (!str) return NULL;

    if (strncmp(str, PREFIX_SELECTED, strlen(PREFIX_SELECTED)) == 0)
        str += strlen(PREFIX_SELECTED);
    else if (strncmp(str, PREFIX_UNSELECTED, strlen(PREFIX_UNSELECTED)) == 0)
        str += strlen(PREFIX_UNSELECTED);

    if (strncmp(str, PREFIX_FAVORITE, strlen(PREFIX_FAVORITE)) == 0)
        str += strlen(PREFIX_FAVORITE);

    if (strncmp(str, PREFIX_ACTIVE, strlen(PREFIX_ACTIVE)) == 0)
        str += strlen(PREFIX_ACTIVE);

    return str;
}

