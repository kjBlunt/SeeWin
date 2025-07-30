#include "include/window_delegate.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <assert.h>
#include <stdio.h>

// Externally defined flag
extern bool terminated;

static void windowWillClose(id self, SEL _sel, id notification)
{
    printf("Window will close\n");
    terminated = true;
}

id setupWindowDelegate()
{
    Class NSObjectClass = objc_getClass("NSObject");

    Class WindowDelegateClass = objc_allocateClassPair(NSObjectClass, "WindowDelegate", 0);
    assert(WindowDelegateClass != Nil);

    Protocol* NSWindowDelegate = objc_getProtocol("NSWindowDelegate");
    bool didAddProtocol = class_addProtocol(WindowDelegateClass, NSWindowDelegate);
    assert(didAddProtocol);

    SEL windowWillCloseSel = sel_registerName("windowWillClose:");
    bool didAddMethod = class_addMethod(
        WindowDelegateClass,
        windowWillCloseSel,
        (IMP)windowWillClose,
        "v@:@"
    );
    assert(didAddMethod);

    objc_registerClassPair(WindowDelegateClass);

    // Instantiate the delegate
    SEL allocSel = sel_registerName("alloc");
    SEL initSel = sel_registerName("init");
    id delegate = ((id (*)(Class, SEL))objc_msgSend)(WindowDelegateClass, allocSel);
    delegate = ((id (*)(id, SEL))objc_msgSend)(delegate, initSel);

    return delegate;
}

