#ifndef LABEL_PREFIXES_H
#define LABEL_PREFIXES_H

extern const char* PREFIX_SELECTED;
extern const char* PREFIX_UNSELECTED;
extern const char* PREFIX_FAVORITE;
extern const char* PREFIX_ACTIVE;

int get_prefix_bit_length(const char* prefix);
const char* strip_prefix(const char* str);

#endif
