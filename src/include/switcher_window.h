#ifndef SWITCHER_WINDOW_H
#define SWITCHER_WINDOW_H

#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <stdbool.h>

extern id appButtons;
extern NSUInteger selectedIndex;

void createSwitcherWindow(id windowDelegate);
id getSwitcherStackView(void);
void updateSelectionHighlight(void);

#endif

