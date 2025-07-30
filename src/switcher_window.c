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

void updateSelectionHighlight()
{
    if (!appButtons) return;

    NSUInteger count = ((NSUInteger (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("count"));
    for (NSUInteger i = 0; i < count; i++) {
        id button = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(appButtons, sel_registerName("objectAtIndex:"), i);

        SEL setFontSel = sel_registerName("setFont:");
        id font = ((id (*)(Class, SEL, double))objc_msgSend)(objc_getClass("NSFont"), sel_registerName("systemFontOfSize:"), (i == selectedIndex) ? 14.0 : 12.0);
        ((void (*)(id, SEL, id))objc_msgSend)(button, setFontSel, font);
    }
}

static void customKeyDown(id self, SEL _cmd, id event)
{
    fprintf(stderr, "appButtons = %p\n", appButtons);

    if (appButtons) {
        SEL countSel = sel_registerName("count");
        NSUInteger count = ((NSUInteger (*)(id, SEL))objc_msgSend)(appButtons, countSel);
        fprintf(stderr, "appButtons count = %lu\n", (unsigned long)count);
    }

    SEL charactersIgnoringModifiersSel = sel_registerName("charactersIgnoringModifiers");
    id chars = ((id (*)(id, SEL))objc_msgSend)(event, charactersIgnoringModifiersSel);

    SEL isEqualToStringSel = sel_registerName("isEqualToString:");
    id qString = ((id (*)(Class, SEL, const char*))objc_msgSend)(
        objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "q"
    );

    const char* cstr = ((const char* (*)(id, SEL))objc_msgSend)(chars, sel_registerName("UTF8String"));
    if (!cstr) return;

    if (strcmp(cstr, "q") == 0) {
        SEL orderOutSel = sel_registerName("orderOut:");
        ((void (*)(id, SEL, id))objc_msgSend)(self, orderOutSel, self);
        return;
    }
    if (appButtons != nil) {
        NSUInteger count = ((NSUInteger (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("count"));
        if (strcmp(cstr, "j") == 0) {
            if (selectedIndex + 1 < count) {
                selectedIndex++;
                updateSelectionHighlight();
            }
            return;
        } else if (strcmp(cstr, "k") == 0) {
            if (selectedIndex > 0) {
                selectedIndex--;
                updateSelectionHighlight();
            }
            return;
        } else if (strcmp(cstr, "\r") == 0 || strcmp(cstr, "\n") == 0) {
            id button = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(appButtons, sel_registerName("objectAtIndex:"), selectedIndex);
            if (!button) return;

            SEL representedObjectSel = sel_registerName("representedObject");
            id app = ((id (*)(id, SEL))objc_msgSend)(button, representedObjectSel);

            if (app) {
                SEL activateSel = sel_registerName("activateWithOptions:");
                ((void (*)(id, SEL, NSUInteger))objc_msgSend)(app, activateSel, 1); // 1 = NSApplicationActivateIgnoringOtherApps

                SEL orderOutSel = sel_registerName("orderOut:");
                ((void (*)(id, SEL, id))objc_msgSend)(window, orderOutSel, window);
            }
            return;
        }
    }

    struct objc_super superInfo = {
        .receiver = self,
        .super_class = class_getSuperclass(object_getClass(self))
    };
    ((void (*)(struct objc_super*, SEL, id))objc_msgSendSuper)(&superInfo, _cmd, event);
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

    // Set window properties
    SEL setTitleSel = sel_registerName("setTitle:");
    id title = ((id (*)(Class, SEL, const char*))objc_msgSend)(
        objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "SeeWin"
    );
    ((void (*)(id, SEL, id))objc_msgSend)(window, setTitleSel, title);

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

    NSRect stackFrame = {{10, 10}, {380, 280}};  // Adjust margins here
    ((void (*)(id, SEL, NSRect))objc_msgSend)(stackViewRef, sel_registerName("setFrame:"), stackFrame);
    ((void (*)(id, SEL, id))objc_msgSend)(scrollView, sel_registerName("setDocumentView:"), paddedView);

    ((void (*)(id, SEL, id))objc_msgSend)(contentView, sel_registerName("addSubview:"), scrollView);
}

id getSwitcherStackView(void)
{
    return stackViewRef;
}
