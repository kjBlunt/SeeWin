#include "include/switcher_window.h"
#include "include/app_list.h"
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

    NSUInteger count = ((NSUInteger (*)(id, SEL))objc_msgSend)(buttons, sel_registerName("count"));

    for (NSUInteger i = 0; i < count; i++) {
        id button = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(buttons, sel_registerName("objectAtIndex:"), i);
        id titleNSString = ((id (*)(id, SEL))objc_msgSend)(button, sel_registerName("title"));
        const char* baseTitle = ((const char* (*)(id, SEL))objc_msgSend)(titleNSString, sel_registerName("UTF8String"));
        if (!baseTitle) continue;

        // Strip leading "> " or "  " from current title
        const char* actualText = (strncmp(baseTitle, "> ", 2) == 0 || strncmp(baseTitle, "  ", 2) == 0)
            ? baseTitle + 2
            : baseTitle;

        BOOL isSelected = isActiveList && (i == selectedOffset);

        char finalLabel[256];
        snprintf(finalLabel, sizeof(finalLabel), isSelected ? "> %s" : "  %s", actualText);

        id newTitle = ((id (*)(Class, SEL, const char*))objc_msgSend)(
            objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), finalLabel
        );
        ((void (*)(id, SEL, id))objc_msgSend)(button, sel_registerName("setTitle:"), newTitle);
    }
}


void updateSelectionHighlight()
{
    if (!appButtons && !favoriteButtons) return;

    updateButtonTitlesForList(favoriteButtons, isInFavoritesList, selectedIndex);
    updateButtonTitlesForList(appButtons, !isInFavoritesList, selectedIndex - totalFavorites);
}

static void activateSelectedApp()
{
    id selectedButton = nil;
    const char* actualText = NULL;

    if (isInFavoritesList && favoriteButtons) {
        NSUInteger favCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(favoriteButtons, sel_registerName("count"));
        if (selectedIndex >= favCount) return;
        selectedButton = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(favoriteButtons, sel_registerName("objectAtIndex:"), selectedIndex);
    } else if (!isInFavoritesList && appButtons) {
        NSUInteger appCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("count"));
        NSUInteger appIndex = selectedIndex - totalFavorites;
        if (appIndex >= appCount) return;
        selectedButton = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(appButtons, sel_registerName("objectAtIndex:"), appIndex);
    }

    if (!selectedButton) return;

    id title = ((id (*)(id, SEL))objc_msgSend)(selectedButton, sel_registerName("title"));
    const char* ctitle = ((const char* (*)(id, SEL))objc_msgSend)(title, sel_registerName("UTF8String"));
    
    // Handle different prefixes: "> ", "  ", "★ ", "* "
    actualText = ctitle;
    
    // Skip selection indicator
    if (strncmp(actualText, "> ", 2) == 0 || strncmp(actualText, "  ", 2) == 0) {
        actualText += 2;
    }
    
    // Skip favorite star
    if (strncmp(actualText, "★ ", 3) == 0) {
        actualText += 4;
    }
    
    // Skip active indicator
    if (strncmp(actualText, "* ", 4) == 0) {
        actualText += 6;
    }

    Class NSWorkspace = objc_getClass("NSWorkspace");
    id workspace = ((id (*)(Class, SEL))objc_msgSend)(NSWorkspace, sel_registerName("sharedWorkspace"));
    id apps = ((id (*)(id, SEL))objc_msgSend)(workspace, sel_registerName("runningApplications"));

    SEL countSel = sel_registerName("count");
    SEL objectAtIndexSel = sel_registerName("objectAtIndex:");
    SEL localizedNameSel = sel_registerName("localizedName");
    SEL activateWithOptionsSel = sel_registerName("activateWithOptions:");
    SEL UTF8StringSel = sel_registerName("UTF8String");

    NSUInteger appCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(apps, countSel);
    for (NSUInteger i = 0; i < appCount; ++i) {
        id app = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(apps, objectAtIndexSel, i);
        id name = ((id (*)(id, SEL))objc_msgSend)(app, localizedNameSel);
        const char* appName = ((const char* (*)(id, SEL))objc_msgSend)(name, UTF8StringSel);

        if (strcmp(appName, actualText) == 0) {
            ((void (*)(id, SEL, NSUInteger))objc_msgSend)(app, activateWithOptionsSel, 1);
            break;
        }
    }

    SEL orderOutSel = sel_registerName("orderOut:");
    ((void (*)(id, SEL, id))objc_msgSend)(window, orderOutSel, window);
}

static void resizeWindowToFitStack()
{
    SEL layoutSel = sel_registerName("layoutSubtreeIfNeeded");
    ((void (*)(id, SEL))objc_msgSend)(stackViewRef, layoutSel);

    SEL fittingSizeSel = sel_registerName("fittingSize");
    NSSize fittingSize = ((NSSize (*)(id, SEL))objc_msgSend)(stackViewRef, fittingSizeSel);

    fittingSize.width += 20;
    fittingSize.height += 20;

    SEL setContentSizeSel = sel_registerName("setContentSize:");
    ((void (*)(id, SEL, NSSize))objc_msgSend)(window, setContentSizeSel, fittingSize);

    ((void (*)(id, SEL))objc_msgSend)(window, sel_registerName("center"));
}


void toggleFavoriteForSelectedApp()
{
    if (!appButtons && !favoriteButtons) return;

    const char* appName = NULL;
    id targetApp = nil;

    if (isInFavoritesList && favoriteButtons) {
        NSUInteger favCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(favoriteButtons, sel_registerName("count"));
        if (selectedIndex >= favCount) return;
        
        id button = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(favoriteButtons, sel_registerName("objectAtIndex:"), selectedIndex);
        id title = ((id (*)(id, SEL))objc_msgSend)(button, sel_registerName("title"));
        const char* ctitle = ((const char* (*)(id, SEL))objc_msgSend)(title, sel_registerName("UTF8String"));
        
        // Parse title - handle selection indicator and star prefix
        appName = ctitle;
        if (strncmp(appName, "> ", 2) == 0 || strncmp(appName, "  ", 2) == 0) {
            appName += 2;
        }
        if (strncmp(appName, "★ ", 3) == 0) {
            appName += 3;
        }

        
        targetApp = ((id (*)(id, SEL))objc_msgSend)(button, sel_registerName("representedObject"));
        
        ((void (*)(id, SEL, NSUInteger))objc_msgSend)(favoriteButtons, sel_registerName("removeObjectAtIndex:"), selectedIndex);
    } else if (!isInFavoritesList && appButtons) {
        NSUInteger appCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("count"));
        NSUInteger appIndex = selectedIndex - totalFavorites;
        if (appIndex >= appCount) return;

        id button = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(appButtons, sel_registerName("objectAtIndex:"), appIndex);
        id title = ((id (*)(id, SEL))objc_msgSend)(button, sel_registerName("title"));
        const char* ctitle = ((const char* (*)(id, SEL))objc_msgSend)(title, sel_registerName("UTF8String"));
        
        // Parse title - handle selection indicator and active indicator
        appName = ctitle;
        if (strncmp(appName, "> ", 2) == 0 || strncmp(appName, "  ", 2) == 0) {
            appName += 2;
        }
        if (strncmp(appName, "* ", 4) == 0) {
            appName += 4;
        }
        
        targetApp = ((id (*)(id, SEL))objc_msgSend)(button, sel_registerName("representedObject"));

        // Add to favorites
        if (!favoriteButtons) {
            favoriteButtons = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSMutableArray"), sel_registerName("alloc"));
            favoriteButtons = ((id (*)(id, SEL))objc_msgSend)(favoriteButtons, sel_registerName("init"));
        }
        
        Class NSButton = objc_getClass("NSButton");
        id favButton = ((id (*)(id, SEL))objc_msgSend)(
            ((id (*)(Class, SEL))objc_msgSend)(NSButton, sel_registerName("alloc")),
            sel_registerName("init")
        );
        
        SEL setButtonTypeSel = sel_registerName("setButtonType:");
        ((void (*)(id, SEL, NSUInteger))objc_msgSend)(favButton, setButtonTypeSel, 0);
        
        SEL setBorderedSel = sel_registerName("setBordered:");
        ((void (*)(id, SEL, BOOL))objc_msgSend)(favButton, setBorderedSel, NO);
        
        SEL setBezelStyleSel = sel_registerName("setBezelStyle:");
        ((void (*)(id, SEL, NSUInteger))objc_msgSend)(favButton, setBezelStyleSel, 15);
        
        SEL setFocusRingTypeSel = sel_registerName("setFocusRingType:");
        ((void (*)(id, SEL, NSUInteger))objc_msgSend)(favButton, setFocusRingTypeSel, 1);
        
        // Set title with ★ prefix for favorites
        char favLabel[256];
        snprintf(favLabel, sizeof(favLabel), "★ %s", appName);
        id favTitle = ((id (*)(Class, SEL, const char*))objc_msgSend)(
            objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), favLabel
        );
        ((void (*)(id, SEL, id))objc_msgSend)(favButton, sel_registerName("setTitle:"), favTitle);
        
        SEL setRepresentedObjectSel = sel_registerName("setRepresentedObject:");
        ((void (*)(id, SEL, id))objc_msgSend)(favButton, setRepresentedObjectSel, targetApp);
        
        ((void (*)(id, SEL, id))objc_msgSend)(favoriteButtons, sel_registerName("addObject:"), favButton);
    }

    updateAppList(stackViewRef);
    resizeWindowToFitStack();
}

static void customKeyDown(id self, SEL _cmd, id event)
{
    SEL charactersIgnoringModifiersSel = sel_registerName("charactersIgnoringModifiers");
    id chars = ((id (*)(id, SEL))objc_msgSend)(event, charactersIgnoringModifiersSel);
    const char* cstr = ((const char* (*)(id, SEL))objc_msgSend)(chars, sel_registerName("UTF8String"));
    if (!cstr) return;

    if (strcmp(cstr, "q") == 0) {
        SEL orderOutSel = sel_registerName("orderOut:");
        ((void (*)(id, SEL, id))objc_msgSend)(self, orderOutSel, self);
        return;
    }

    if (strcmp(cstr, "f") == 0) {
        toggleFavoriteForSelectedApp();
        return;
    }

    NSUInteger totalItems = totalFavorites;
    if (appButtons) {
        totalItems += ((NSUInteger (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("count"));
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
    if (strcmp(cstr, "j") == 0) {
        if (selectedIndex + 1 < totalItems) {
            selectedIndex++;
            isInFavoritesList = (selectedIndex < totalFavorites);
            updateSelectionHighlight();
        }
        return;
    } else if (strcmp(cstr, "k") == 0) {
        if (selectedIndex > 0) {
            selectedIndex--;
            isInFavoritesList = (selectedIndex < totalFavorites);
            updateSelectionHighlight();
        }
        return;
    } else if (strcmp(cstr, "\r") == 0 || strcmp(cstr, "\n") == 0) {
        activateSelectedApp();
        return;
    }

    struct objc_super superInfo = {
        .receiver = self,
        .super_class = class_getSuperclass(object_getClass(self))
    };
    ((void (*)(struct objc_super*, SEL, id))objc_msgSendSuper)(&superInfo, _cmd, event);
}

static void toggleVisibility(id self, SEL _cmd)
{
    SEL isVisibleSel = sel_registerName("isVisible");
    BOOL visible = ((BOOL (*)(id, SEL))objc_msgSend)(self, isVisibleSel);

    if (visible) {
        SEL orderOutSel = sel_registerName("orderOut:");
        ((void (*)(id, SEL, id))objc_msgSend)(self, orderOutSel, self);
    } else {
        updateAppList(stackViewRef);
        resizeWindowToFitStack();

        selectedIndex = 0;
        isInFavoritesList = (totalFavorites > 0);

        SEL makeKeyAndOrderFrontSel = sel_registerName("makeKeyAndOrderFront:");
        ((void (*)(id, SEL, id))objc_msgSend)(self, makeKeyAndOrderFrontSel, self);

        SEL setAcceptsMouseMovedEventsSel = sel_registerName("setAcceptsMouseMovedEvents:");
        ((void (*)(id, SEL, BOOL))objc_msgSend)(window, setAcceptsMouseMovedEventsSel, YES);

        SEL setIgnoresMouseEventsSel = sel_registerName("setIgnoresMouseEvents:");
        ((void (*)(id, SEL, BOOL))objc_msgSend)(window, setIgnoresMouseEventsSel, NO);

        SEL setAcceptsFirstResponderSel = sel_registerName("makeFirstResponder:");
        ((void (*)(id, SEL, id))objc_msgSend)(window, setAcceptsFirstResponderSel, window);

        SEL activateIgnoringOtherAppsSel = sel_registerName("activateIgnoringOtherApps:");
        extern id NSApp;
        ((void (*)(id, SEL, BOOL))objc_msgSend)(NSApp, activateIgnoringOtherAppsSel, YES);
    }
}

BOOL acceptsFirstResponder(id self, SEL _cmd) {
  return YES;
}

void createSwitcherWindow(id windowDelegate)
{
    Class NSWindowClass = objc_getClass("NSWindow");
    Class SwitcherWindowClass = objc_allocateClassPair(NSWindowClass, "SwitcherWindow", 0);
    assert(SwitcherWindowClass != Nil);

    class_addMethod(SwitcherWindowClass, sel_registerName("keyDown:"), (IMP)customKeyDown, "v@:@");
    class_addMethod(SwitcherWindowClass, sel_registerName("toggleVisibility"), (IMP)toggleVisibility, "v@:");
    class_addMethod(SwitcherWindowClass, sel_registerName("acceptsFirstResponder"), (IMP)acceptsFirstResponder, "c@:");
    objc_registerClassPair(SwitcherWindowClass);

    NSRect rect = {{100, 100}, {400, 300}};
    SEL allocSel = sel_registerName("alloc");
    SEL initSel = sel_registerName("initWithContentRect:styleMask:backing:defer:");

    id windowAlloc = ((id (*)(Class, SEL))objc_msgSend)(SwitcherWindowClass, allocSel);
    NSUInteger styleMask = (1 << 0); // NSWindowStyleMaskBorderless
    window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(
        windowAlloc, initSel, rect, styleMask, 2, NO
    );

    ((void (*)(id, SEL, BOOL))objc_msgSend)(window, sel_registerName("setOpaque:"), NO);
    id clearColor = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSColor"), sel_registerName("clearColor"));
    ((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("setBackgroundColor:"), clearColor);

    ((void (*)(id, SEL, NSInteger))objc_msgSend)(window, sel_registerName("setTitleVisibility:"), 1); // NSWindowTitleHidden
    ((void (*)(id, SEL, BOOL))objc_msgSend)(window, sel_registerName("setTitlebarAppearsTransparent:"), YES);

    // Set window level above normal windows
    ((void (*)(id, SEL, NSInteger))objc_msgSend)(window, sel_registerName("setLevel:"), 3); // NSStatusWindowLevel

    ((void (*)(id, SEL, BOOL))objc_msgSend)(window, sel_registerName("setHasShadow:"), NO);

    SEL centerSel = sel_registerName("center");
    ((void (*)(id, SEL))objc_msgSend)(window, centerSel);

    SEL setDelegateSel = sel_registerName("setDelegate:");
    ((void (*)(id, SEL, id))objc_msgSend)(window, setDelegateSel, windowDelegate);

    SEL setReleasedWhenClosedSel = sel_registerName("setReleasedWhenClosed:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(window, setReleasedWhenClosedSel, NO);

    SEL setCollectionBehaviorSel = sel_registerName("setCollectionBehavior:");
    NSUInteger behavior = (1 << 1);
    ((void (*)(id, SEL, NSUInteger))objc_msgSend)(window, setCollectionBehaviorSel, behavior);

    SEL contentViewSel = sel_registerName("contentView");
    id contentView = ((id (*)(id, SEL))objc_msgSend)(window, contentViewSel);

    // Create NSVisualEffectView for blur
    Class NSVisualEffectView = objc_getClass("NSVisualEffectView");
    id blurView = ((id (*)(Class, SEL))objc_msgSend)(NSVisualEffectView, sel_registerName("alloc"));
    blurView = ((id (*)(id, SEL))objc_msgSend)(blurView, sel_registerName("init"));

    ((void (*)(id, SEL, NSInteger))objc_msgSend)(blurView, sel_registerName("setMaterial:"), 0); // NSVisualEffectMaterialAppearanceBased
    ((void (*)(id, SEL, NSInteger))objc_msgSend)(blurView, sel_registerName("setBlendingMode:"), 0); // BehindWindow
    ((void (*)(id, SEL, NSInteger))objc_msgSend)(blurView, sel_registerName("setState:"), 1); // FollowsWindowActiveState

    ((void (*)(id, SEL, id))objc_msgSend)(window, sel_registerName("setContentView:"), blurView);

    ((void (*)(id, SEL, BOOL))objc_msgSend)(blurView, sel_registerName("setWantsLayer:"), YES);
    id layer = ((id (*)(id, SEL))objc_msgSend)(blurView, sel_registerName("layer"));
    ((void (*)(id, SEL, CGFloat))objc_msgSend)(layer, sel_registerName("setCornerRadius:"), 12.0);
    ((void (*)(id, SEL, BOOL))objc_msgSend)(layer, sel_registerName("setMasksToBounds:"), YES);

    NSRect scrollFrame = {{0, 0}, {400, 300}};
    NSRect textFrame = {{0, 0}, {380, 300}};

    Class NSScrollView = objc_getClass("NSScrollView");
    id scrollView = ((id (*)(id, SEL))objc_msgSend)(
        ((id (*)(Class, SEL))objc_msgSend)(NSScrollView, sel_registerName("alloc")),
        sel_registerName("init")
    );

    ((void (*)(id, SEL, NSRect))objc_msgSend)(scrollView, sel_registerName("setFrame:"), scrollFrame);
    ((void (*)(id, SEL, BOOL))objc_msgSend)(scrollView, sel_registerName("setHasVerticalScroller:"), YES);

    Class NSStackView = objc_getClass("NSStackView");
    id stackView = ((id (*)(Class, SEL))objc_msgSend)(NSStackView, sel_registerName("alloc"));
    stackView = ((id (*)(id, SEL))objc_msgSend)(stackView, sel_registerName("init"));

    SEL setOrientationSel = sel_registerName("setOrientation:");
    ((void (*)(id, SEL, NSInteger))objc_msgSend)(stackView, setOrientationSel, 1);
    stackViewRef = stackView;

    SEL setTranslatesAutoresizingMaskIntoConstraintsSel = sel_registerName("setTranslatesAutoresizingMaskIntoConstraints:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(stackView, setTranslatesAutoresizingMaskIntoConstraintsSel, NO);

    SEL setAutoresizingMaskSel = sel_registerName("setAutoresizingMask:");
    NSUInteger mask = (1 << 1) | (1 << 4); // NSViewWidthSizable | NSViewHeightSizable
    ((void (*)(id, SEL, NSUInteger))objc_msgSend)(stackView, setAutoresizingMaskSel, mask);

    SEL setAlignmentSel = sel_registerName("setAlignment:");
    ((void (*)(id, SEL, NSUInteger))objc_msgSend)(stackView, setAlignmentSel, 1); // NSLayoutAttributeLeading

    SEL setHuggingSel = sel_registerName("setHuggingPriority:forOrientation:");
    ((void (*)(id, SEL, float, NSUInteger))objc_msgSend)(stackView, setHuggingSel, 251.0f, 1); // vertical orientation

    SEL setSpacingSel = sel_registerName("setSpacing:");
    ((void (*)(id, SEL, double))objc_msgSend)(stackView, setSpacingSel, 8.0);

    Class NSView = objc_getClass("NSView");
    id paddedView = ((id (*)(Class, SEL))objc_msgSend)(NSView, sel_registerName("alloc"));
    paddedView = ((id (*)(id, SEL))objc_msgSend)(paddedView, sel_registerName("init"));

    ((void (*)(id, SEL, NSRect))objc_msgSend)(paddedView, sel_registerName("setFrame:"), scrollFrame);

    SEL addSubviewSel = sel_registerName("addSubview:");
    ((void (*)(id, SEL, id))objc_msgSend)(paddedView, addSubviewSel, stackViewRef);

    NSRect stackFrame = {{10, 10}, {380, 280}};
    ((void (*)(id, SEL, NSRect))objc_msgSend)(stackViewRef, sel_registerName("setFrame:"), stackFrame);
    ((void (*)(id, SEL, id))objc_msgSend)(scrollView, sel_registerName("setDocumentView:"), paddedView);

    ((void (*)(id, SEL, id))objc_msgSend)(blurView, sel_registerName("addSubview:"), scrollView);
}

id getSwitcherStackView(void)
{
    return stackViewRef;
}
