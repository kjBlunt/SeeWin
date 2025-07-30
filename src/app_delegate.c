#include "include/app_delegate.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <stdio.h>
#include <assert.h>

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
#define NSIntegerEncoding "q"
#define NSUIntegerEncoding "L"
#else
#define NSIntegerEncoding "i"
#define NSUIntegerEncoding "I"
#endif

// Termination flag from main app
extern bool terminated;

static NSUInteger applicationShouldTerminate()
{
    printf("Application termination requested\n");
    terminated = true;
    return 0;
}

id setupAppDelegate()
{
    Class NSObjectClass = objc_getClass("NSObject");

    Class AppDelegateClass = objc_allocateClassPair(NSObjectClass, "AppDelegate", 0);
    assert(AppDelegateClass != Nil);

    Protocol* NSApplicationDelegate = objc_getProtocol("NSApplicationDelegate");
    bool didAddProtocol = class_addProtocol(AppDelegateClass, NSApplicationDelegate);
    assert(didAddProtocol);

    SEL selTerminate = sel_registerName("applicationShouldTerminate:");
    bool didAddMethod = class_addMethod(
        AppDelegateClass,
        selTerminate,
        (IMP)applicationShouldTerminate,
        NSUIntegerEncoding "@:@"
    );
    assert(didAddMethod);

    objc_registerClassPair(AppDelegateClass);

    // Create and return an instance
    SEL allocSel = sel_registerName("alloc");
    SEL initSel = sel_registerName("init");
    id delegate = ((id (*)(Class, SEL))objc_msgSend)(AppDelegateClass, allocSel);
    delegate = ((id (*)(id, SEL))objc_msgSend)(delegate, initSel);

    return delegate;
}

