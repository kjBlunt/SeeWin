#include "include/window_delegate.h"
#include "include/macros.h"
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

    SEL windowWillCloseSel = SEL("windowWillClose:");
    bool didAddMethod = class_addMethod(
        WindowDelegateClass,
        windowWillCloseSel,
        (IMP)windowWillClose,
        "v@:@"
    );
    assert(didAddMethod);

    objc_registerClassPair(WindowDelegateClass);

    // Instantiate the delegate
    SEL allocSel = SEL_ALLOC;
    SEL initSel = SEL_INIT;
    id delegate = OBJC_CLASS_CALL_ID(WindowDelegateClass, allocSel);
    delegate = OBJC_CALL_ID(delegate, initSel);

    return delegate;
}

