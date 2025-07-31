#include "include/switcher_window.h"
#include "include/app_list.h"
#include "include/macros.h"
#include "include/label_prefixes.h"
#include "include/config.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <CoreGraphics/CGGeometry.h>
#include <objc/NSObjCRuntime.h>


typedef CGPoint NSPoint;
typedef CGRect NSRect;
typedef struct CGSize NSSize;

id window = nil;
static id stackViewRef = nil;
NSUInteger selectedIndex = 0;
id appButtons = nil;
id favoriteButtons = nil;
NSUInteger totalFavorites = 0;
BOOL isInFavoritesList = NO;

static void updateButtonTitlesForList(id buttons, BOOL isActiveList, NSUInteger selectedOffset) {
    if (!buttons) return;

    NSUInteger count = OBJC_CALL_UINT(buttons, SEL_COUNT);

    for (NSUInteger i = 0; i < count; i++) {
        id button = OBJC_CALL_ID_UINT(buttons, SEL_OBJECT_AT_INDEX, i);
        id titleNSString = OBJC_CALL_ID(button, SEL_TITLE);
        const char* baseTitle = OBJC_CALL_CSTRING(titleNSString, SEL_UTF8_STRING);
        if (!baseTitle) continue;

        const char* rawText = strip_prefix(baseTitle);

        BOOL wasFavorite = strstr(baseTitle, PREFIX_FAVORITE) != NULL;
        BOOL wasActive = strstr(baseTitle, PREFIX_ACTIVE) != NULL;
        BOOL isSelected = isActiveList && (i == selectedOffset);

        char decoratedTitle[256];
        snprintf(decoratedTitle, sizeof(decoratedTitle), "%s%s%s%s",
                 isSelected ? PREFIX_SELECTED : PREFIX_UNSELECTED,
                 wasFavorite ? PREFIX_FAVORITE : "",
                 wasActive ? PREFIX_ACTIVE : "",
                 rawText);

        id newTitle = OBJC_CLASS_CALL_ID_CSTRING(objc_getClass("NSString"), SEL_STRING_WITH_UTF8_STRING, decoratedTitle);
        OBJC_CALL_VOID_ARG(button, SEL_SET_TITLE, newTitle);
    }
}


void updateSelectionHighlight() {
    if (!appButtons && !favoriteButtons) return;

    updateButtonTitlesForList(favoriteButtons, isInFavoritesList, selectedIndex);
    updateButtonTitlesForList(appButtons, !isInFavoritesList, selectedIndex - totalFavorites);
}

static void activateSelectedApp() {
    id selectedButton = nil;
    const char* actualText = NULL;

    if (isInFavoritesList && favoriteButtons) {
        NSUInteger favCount = OBJC_CALL_UINT(favoriteButtons, SEL_COUNT);
        if (selectedIndex >= favCount) return;
        selectedButton = OBJC_CALL_ID_UINT(favoriteButtons, SEL_OBJECT_AT_INDEX, selectedIndex);
    } else if (!isInFavoritesList && appButtons) {
        NSUInteger appCount = OBJC_CALL_UINT(appButtons, SEL_COUNT);
        NSUInteger appIndex = selectedIndex - totalFavorites;
        if (appIndex >= appCount) return;
        selectedButton = OBJC_CALL_ID_UINT(appButtons, SEL_OBJECT_AT_INDEX, appIndex);
    }

    if (!selectedButton) return;

    id title = OBJC_CALL_ID(selectedButton, SEL_TITLE);
    const char* ctitle = OBJC_CALL_CSTRING(title, SEL_UTF8_STRING);
    
    actualText = strip_prefix(ctitle);

    Class NSWorkspace = objc_getClass("NSWorkspace");
    id workspace = OBJC_CLASS_CALL_ID(NSWorkspace, SEL("sharedWorkspace"));
    id apps = OBJC_CALL_ID(workspace, SEL("runningApplications"));

    SEL countSel = SEL_COUNT;
    SEL objectAtIndexSel = SEL_OBJECT_AT_INDEX;
    SEL localizedNameSel = SEL("localizedName");
    SEL activateWithOptionsSel = SEL("activateWithOptions:");
    SEL UTF8StringSel = SEL_UTF8_STRING;

    NSUInteger appCount = OBJC_CALL_UINT(apps, countSel);
    for (NSUInteger i = 0; i < appCount; ++i) {
        id app = OBJC_CALL_ID_UINT(apps, objectAtIndexSel, i);
        id name = OBJC_CALL_ID(app, localizedNameSel);
        const char* appName = OBJC_CALL_CSTRING(name, UTF8StringSel);

        if (strcmp(appName, actualText) == 0) {
            OBJC_CALL_VOID_UINT(app, activateWithOptionsSel, 1);
            break;
        }
    }

    SEL orderOutSel = SEL("orderOut:");
    OBJC_CALL_VOID_ARG(window, orderOutSel, window);
}

static void resizeWindowToFitStack() {
    SEL layoutSel = SEL("layoutSubtreeIfNeeded");
    OBJC_CALL_VOID(stackViewRef, layoutSel);

    SEL fittingSizeSel = SEL("fittingSize");
    NSSize fittingSize = OBJC_CALL_NSSIZE(stackViewRef, fittingSizeSel);

    fittingSize.width += 20;
    fittingSize.height += 20;

    SEL setContentSizeSel = SEL("setContentSize:");
    OBJC_CALL_VOID_SIZE(window, setContentSizeSel, fittingSize);

    OBJC_CALL_VOID(window, SEL("center"));
}


void toggleFavoriteForSelectedApp() {
    if (!appButtons && !favoriteButtons) return;

    const char* appName = NULL;
    id targetApp = nil;

    if (isInFavoritesList && favoriteButtons) {
        NSUInteger favCount = OBJC_CALL_UINT(favoriteButtons, SEL_COUNT);
        if (selectedIndex >= favCount) return;
        
        id button = OBJC_CALL_ID_UINT(favoriteButtons, SEL_OBJECT_AT_INDEX, selectedIndex);
        id title = OBJC_CALL_ID(button, SEL_TITLE);
        const char* ctitle = OBJC_CALL_CSTRING(title, SEL_UTF8_STRING);
        
        appName = strip_prefix(ctitle);
        
        targetApp = OBJC_CALL_ID(button, SEL_REPRESENTED_OBJECT);
        
        OBJC_CALL_VOID_UINT(favoriteButtons, SEL_REMOVE_OBJECT_AT_INDEX, selectedIndex);
    } else if (!isInFavoritesList && appButtons) {
        NSUInteger appCount = OBJC_CALL_UINT(appButtons, SEL_COUNT);
        NSUInteger appIndex = selectedIndex - totalFavorites;
        if (appIndex >= appCount) return;

        id button = OBJC_CALL_ID_UINT(appButtons, SEL_OBJECT_AT_INDEX, appIndex);
        id title = OBJC_CALL_ID(button, SEL_TITLE);
        const char* ctitle = OBJC_CALL_CSTRING(title, SEL_UTF8_STRING);
        
        appName = strip_prefix(ctitle);
        
        targetApp = OBJC_CALL_ID(button, SEL_REPRESENTED_OBJECT);

        // Add to favorites
        if (!favoriteButtons) {
            favoriteButtons = OBJC_CLASS_CALL_ID(objc_getClass("NSMutableArray"), SEL_ALLOC);
            favoriteButtons = OBJC_CALL_ID(favoriteButtons, SEL_INIT);
        }
        
        Class NSButton = objc_getClass("NSButton");
        id favButton = OBJC_CALL_ID(OBJC_CLASS_CALL_ID(NSButton, SEL_ALLOC), SEL_INIT);
        
        SEL setButtonTypeSel = SEL("setButtonType:");
        OBJC_CALL_VOID_UINT(favButton, setButtonTypeSel, 0);
        
        SEL setBorderedSel = SEL("setBordered:");
        OBJC_CALL_VOID_BOOL(favButton, setBorderedSel, NO);
        
        SEL setBezelStyleSel = SEL("setBezelStyle:");
        OBJC_CALL_VOID_UINT(favButton, setBezelStyleSel, 15);
        
        SEL setFocusRingTypeSel = SEL("setFocusRingType:");
        OBJC_CALL_VOID_UINT(favButton, setFocusRingTypeSel, 1);
        
        BOOL wasActive = strstr(ctitle, PREFIX_ACTIVE) != NULL;

        char favLabel[256];
        snprintf(favLabel, sizeof(favLabel), "%s%s%s",
                 PREFIX_FAVORITE,
                 wasActive ? PREFIX_ACTIVE : "",
                 appName);

        id favTitle = OBJC_CLASS_CALL_ID_CSTRING(objc_getClass("NSString"), SEL_STRING_WITH_UTF8_STRING, favLabel);
        OBJC_CALL_VOID_ARG(favButton, SEL_SET_TITLE, favTitle);
        
        SEL setRepresentedObjectSel = SEL_SET_REPRESENTED_OBJECT;
        OBJC_CALL_VOID_ARG(favButton, setRepresentedObjectSel, targetApp);
        
        OBJC_CALL_VOID_ARG(favoriteButtons, SEL_ADD_OBJECT, favButton);
    }

    updateAppList(stackViewRef);
    resizeWindowToFitStack();
}

static void customKeyDown(id self, SEL _cmd, id event) {
    SEL charactersIgnoringModifiersSel = SEL("charactersIgnoringModifiers");
    id chars = OBJC_CALL_ID(event, charactersIgnoringModifiersSel);
    const char* cstr = OBJC_CALL_CSTRING(chars, SEL_UTF8_STRING);
    if (!cstr) return;

    if (cstr[0] == KEY_QUIT) {
        SEL orderOutSel = SEL("orderOut:");
        OBJC_CALL_VOID_ARG(self, orderOutSel, self);
        return;
    }

    if (cstr[0] == KEY_FAVORITE) {
        toggleFavoriteForSelectedApp();
        return;
    }

    NSUInteger totalItems = totalFavorites;
    if (appButtons) {
        totalItems += OBJC_CALL_UINT(appButtons, SEL_COUNT);
    }

    // Handle number keys (0-9)
    if (cstr[1] == '\0' && cstr[0] >= '0' && cstr[0] <= '9') {
        NSUInteger index = (cstr[0] == '0') ? 9 : (cstr[0] - '1');
        if (index < totalItems) {
            selectedIndex = index;
            isInFavoritesList = (index < totalFavorites);
            updateSelectionHighlight();
            activateSelectedApp();
        }
        return;
    }

    // Handle navigation keys
    if (cstr[0] == KEY_UP) {
        if (selectedIndex + 1 < totalItems) {
            selectedIndex++;
            isInFavoritesList = (selectedIndex < totalFavorites);
            updateSelectionHighlight();
        }
        return;
    } else if (cstr[0] == KEY_DOWN) {
        if (selectedIndex > 0) {
            selectedIndex--;
            isInFavoritesList = (selectedIndex < totalFavorites);
            updateSelectionHighlight();
        }
        return;
    } else if (cstr[0] == KEY_ACTIVATE) {
        activateSelectedApp();
        return;
    }

    if (cstr[0] == KEY_RELOAD) {
        load_config();
        return;
    }

    struct objc_super superInfo = {
        .receiver = self,
        .super_class = class_getSuperclass(object_getClass(self))
    };
    OBJC_SUPER_CALL_VOID_ARG(&superInfo, _cmd, event);
}

static void toggleVisibility(id self, SEL _cmd) {
    SEL isVisibleSel = SEL("isVisible");
    BOOL visible = OBJC_CALL_BOOL(self, isVisibleSel);

    if (visible) {
        SEL orderOutSel = SEL("orderOut:");
        OBJC_CALL_VOID_ARG(self, orderOutSel, self);
    } else {
        updateAppList(stackViewRef);
        resizeWindowToFitStack();

        selectedIndex = 0;
        isInFavoritesList = (totalFavorites > 0);

        SEL makeKeyAndOrderFrontSel = SEL("makeKeyAndOrderFront:");
        OBJC_CALL_VOID_ARG(self, makeKeyAndOrderFrontSel, self);

        SEL setAcceptsMouseMovedEventsSel = SEL("setAcceptsMouseMovedEvents:");
        OBJC_CALL_VOID_BOOL(window, setAcceptsMouseMovedEventsSel, YES);

        SEL setIgnoresMouseEventsSel = SEL("setIgnoresMouseEvents:");
        OBJC_CALL_VOID_BOOL(window, setIgnoresMouseEventsSel, NO);

        SEL setAcceptsFirstResponderSel = SEL("makeFirstResponder:");
        OBJC_CALL_VOID_ARG(window, setAcceptsFirstResponderSel, window);

        SEL activateIgnoringOtherAppsSel = SEL("activateIgnoringOtherApps:");
        extern id NSApp;
        OBJC_CALL_VOID_BOOL(NSApp, activateIgnoringOtherAppsSel, YES);
    }
}

BOOL acceptsFirstResponder(id self, SEL _cmd) {
  return YES;
}

void createSwitcherWindow(id windowDelegate) {
    Class NSWindowClass = objc_getClass("NSWindow");
    Class SwitcherWindowClass = objc_allocateClassPair(NSWindowClass, "SwitcherWindow", 0);
    assert(SwitcherWindowClass != Nil);

    class_addMethod(SwitcherWindowClass, SEL("keyDown:"), (IMP)customKeyDown, "v@:@");
    class_addMethod(SwitcherWindowClass, SEL("toggleVisibility"), (IMP)toggleVisibility, "v@:");
    class_addMethod(SwitcherWindowClass, SEL("acceptsFirstResponder"), (IMP)acceptsFirstResponder, "c@:");
    objc_registerClassPair(SwitcherWindowClass);

    NSRect rect = {{100, 100}, {400, 300}};
    SEL allocSel = SEL_ALLOC;
    SEL initSel = SEL("initWithContentRect:styleMask:backing:defer:");

    id windowAlloc = OBJC_CLASS_CALL_ID(SwitcherWindowClass, allocSel);
    NSUInteger styleMask = (1 << 0); // NSWindowStyleMaskBorderless
    window = OBJC_CALL_ID_RECT_UINT_UINT_BOOL(windowAlloc, initSel, rect, styleMask, 2, NO);

    OBJC_CALL_VOID_BOOL(window, SEL("setOpaque:"), NO);
    id clearColor = OBJC_CLASS_CALL_ID(objc_getClass("NSColor"), SEL("clearColor"));
    OBJC_CALL_VOID_ARG(window, SEL("setBackgroundColor:"), clearColor);

    OBJC_CALL_VOID_INT(window, SEL("setTitleVisibility:"), 1); // NSWindowTitleHidden
    OBJC_CALL_VOID_BOOL(window, SEL("setTitlebarAppearsTransparent:"), YES);

    // Set window level above normal windows
    OBJC_CALL_VOID_INT(window, SEL("setLevel:"), 3); // NSStatusWindowLevel

    OBJC_CALL_VOID_BOOL(window, SEL("setHasShadow:"), NO);

    SEL centerSel = SEL("center");
    OBJC_CALL_VOID(window, centerSel);

    SEL setDelegateSel = SEL("setDelegate:");
    OBJC_CALL_VOID_ARG(window, setDelegateSel, windowDelegate);

    SEL setReleasedWhenClosedSel = SEL("setReleasedWhenClosed:");
    OBJC_CALL_VOID_BOOL(window, setReleasedWhenClosedSel, NO);

    SEL setCollectionBehaviorSel = SEL("setCollectionBehavior:");
    NSUInteger behavior = (1 << 1);
    OBJC_CALL_VOID_UINT(window, setCollectionBehaviorSel, behavior);

    SEL contentViewSel = SEL("contentView");
    id contentView = OBJC_CALL_ID(window, contentViewSel);

    // Create NSVisualEffectView for blur
    Class NSVisualEffectView = objc_getClass("NSVisualEffectView");
    id blurView = OBJC_CALL_ID(OBJC_CLASS_CALL_ID(NSVisualEffectView, SEL_ALLOC), SEL_INIT);

    OBJC_CALL_VOID_INT(blurView, SEL("setMaterial:"), 0); // NSVisualEffectMaterialAppearanceBased
    OBJC_CALL_VOID_INT(blurView, SEL("setBlendingMode:"), 0); // BehindWindow
    OBJC_CALL_VOID_INT(blurView, SEL("setState:"), 1); // FollowsWindowActiveState

    OBJC_CALL_VOID_ARG(window, SEL("setContentView:"), blurView);

    OBJC_CALL_VOID_BOOL(blurView, SEL("setWantsLayer:"), YES);
    id layer = OBJC_CALL_ID(blurView, SEL("layer"));
    OBJC_CALL_VOID_FLOAT(layer, SEL("setCornerRadius:"), 12.0);
    OBJC_CALL_VOID_BOOL(layer, SEL("setMasksToBounds:"), YES);

    NSRect scrollFrame = {{0, 0}, {400, 300}};
    NSRect textFrame = {{0, 0}, {380, 300}};

    Class NSScrollView = objc_getClass("NSScrollView");
    id scrollView = OBJC_CALL_ID(OBJC_CLASS_CALL_ID(NSScrollView, SEL_ALLOC), SEL_INIT);

    OBJC_CALL_VOID_RECT(scrollView, SEL("setFrame:"), scrollFrame);
    OBJC_CALL_VOID_BOOL(scrollView, SEL("setHasVerticalScroller:"), YES);

    Class NSStackView = objc_getClass("NSStackView");
    id stackView = OBJC_CALL_ID(OBJC_CLASS_CALL_ID(NSStackView, SEL_ALLOC), SEL_INIT);

    SEL setOrientationSel = SEL("setOrientation:");
    OBJC_CALL_VOID_INT(stackView, setOrientationSel, 1);
    stackViewRef = stackView;

    SEL setTranslatesAutoresizingMaskIntoConstraintsSel = SEL("setTranslatesAutoresizingMaskIntoConstraints:");
    OBJC_CALL_VOID_BOOL(stackView, setTranslatesAutoresizingMaskIntoConstraintsSel, NO);

    SEL setAutoresizingMaskSel = SEL("setAutoresizingMask:");
    NSUInteger mask = (1 << 1) | (1 << 4); // NSViewWidthSizable | NSViewHeightSizable
    OBJC_CALL_VOID_UINT(stackView, setAutoresizingMaskSel, mask);

    SEL setAlignmentSel = SEL("setAlignment:");
    OBJC_CALL_VOID_UINT(stackView, setAlignmentSel, 1); // NSLayoutAttributeLeading

    SEL setHuggingSel = SEL("setHuggingPriority:forOrientation:");
    OBJC_CALL_VOID_FLOAT_UINT(stackView, setHuggingSel, 251.0f, 1); // vertical orientation

    SEL setSpacingSel = SEL("setSpacing:");
    OBJC_CALL_VOID_DOUBLE(stackView, setSpacingSel, 8.0);

    Class NSView = objc_getClass("NSView");
    id paddedView = OBJC_CALL_ID(OBJC_CLASS_CALL_ID(NSView, SEL_ALLOC), SEL_INIT);

    OBJC_CALL_VOID_RECT(paddedView, SEL("setFrame:"), scrollFrame);

    SEL addSubviewSel = SEL("addSubview:");
    OBJC_CALL_VOID_ARG(paddedView, addSubviewSel, stackViewRef);

    NSRect stackFrame = {{10, 10}, {380, 280}};
    OBJC_CALL_VOID_RECT(stackViewRef, SEL("setFrame:"), stackFrame);
    OBJC_CALL_VOID_ARG(scrollView, SEL("setDocumentView:"), paddedView);

    OBJC_CALL_VOID_ARG(blurView, SEL("addSubview:"), scrollView);
}

id getSwitcherStackView(void)
{
    return stackViewRef;
}
