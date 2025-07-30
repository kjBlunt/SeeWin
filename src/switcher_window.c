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

id window = nil;
static id textViewRef = nil;

static void customKeyDown(id self, SEL _cmd, id event)
{
    SEL charactersIgnoringModifiersSel = sel_registerName("charactersIgnoringModifiers");
    id chars = ((id (*)(id, SEL))objc_msgSend)(event, charactersIgnoringModifiersSel);

    SEL isEqualToStringSel = sel_registerName("isEqualToString:");
    id qString = ((id (*)(Class, SEL, const char*))objc_msgSend)(
        objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "q"
    );

    BOOL isQ = ((BOOL (*)(id, SEL, id))objc_msgSend)(chars, isEqualToStringSel, qString);

    if (isQ) {
        SEL orderOutSel = sel_registerName("orderOut:");
        ((void (*)(id, SEL, id))objc_msgSend)(self, orderOutSel, self);
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
        updateAppList(textViewRef);

        SEL makeKeyAndOrderFrontSel = sel_registerName("makeKeyAndOrderFront:");
        ((void (*)(id, SEL, id))objc_msgSend)(self, makeKeyAndOrderFrontSel, self);

        SEL activateIgnoringOtherAppsSel = sel_registerName("activateIgnoringOtherApps:");
        extern id NSApp;
        ((void (*)(id, SEL, BOOL))objc_msgSend)(NSApp, activateIgnoringOtherAppsSel, YES);
    }
}

void createSwitcherWindow(id windowDelegate)
{
    Class NSWindowClass = objc_getClass("NSWindow");
    Class SwitcherWindowClass = objc_allocateClassPair(NSWindowClass, "SwitcherWindow", 0);
    assert(SwitcherWindowClass != Nil);

    class_addMethod(SwitcherWindowClass, sel_registerName("keyDown:"), (IMP)customKeyDown, "v@:@");
    class_addMethod(SwitcherWindowClass, sel_registerName("toggleVisibility"), (IMP)toggleVisibility, "v@:");
    objc_registerClassPair(SwitcherWindowClass);

    NSRect rect = {{100, 100}, {400, 300}};
    SEL allocSel = sel_registerName("alloc");
    SEL initSel = sel_registerName("initWithContentRect:styleMask:backing:defer:");

    id windowAlloc = ((id (*)(Class, SEL))objc_msgSend)(SwitcherWindowClass, allocSel);
    window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(
        windowAlloc, initSel, rect, 0, 2, NO
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
    NSUInteger behavior = (1 << 1); // MoveToActiveSpace
    ((void (*)(id, SEL, NSUInteger))objc_msgSend)(window, setCollectionBehaviorSel, behavior);

    // Content view
    SEL contentViewSel = sel_registerName("contentView");
    id contentView = ((id (*)(id, SEL))objc_msgSend)(window, contentViewSel);

    // ScrollView and TextView
    NSRect scrollFrame = {{0, 0}, {400, 300}};
    NSRect textFrame = {{0, 0}, {380, 300}};

    Class NSScrollView = objc_getClass("NSScrollView");
    id scrollView = ((id (*)(id, SEL))objc_msgSend)(
        ((id (*)(Class, SEL))objc_msgSend)(NSScrollView, sel_registerName("alloc")),
        sel_registerName("init")
    );

    ((void (*)(id, SEL, NSRect))objc_msgSend)(scrollView, sel_registerName("setFrame:"), scrollFrame);
    ((void (*)(id, SEL, BOOL))objc_msgSend)(scrollView, sel_registerName("setHasVerticalScroller:"), YES);

    Class NSTextView = objc_getClass("NSTextView");
    id textView = ((id (*)(id, SEL, NSRect))objc_msgSend)(
        ((id (*)(Class, SEL))objc_msgSend)(NSTextView, sel_registerName("alloc")),
        sel_registerName("initWithFrame:"),
        textFrame
    );
    textViewRef = textView;

    ((void (*)(id, SEL, BOOL))objc_msgSend)(textView, sel_registerName("setEditable:"), NO);
    ((void (*)(id, SEL, BOOL))objc_msgSend)(textView, sel_registerName("setSelectable:"), NO);

    ((void (*)(id, SEL, id))objc_msgSend)(scrollView, sel_registerName("setDocumentView:"), textView);
    ((void (*)(id, SEL, id))objc_msgSend)(contentView, sel_registerName("addSubview:"), scrollView);

    // Save reference to textView if needed later (store in a global or expose accessor)
}

id getSwitcherTextView(void)
{
    return textViewRef;
}
