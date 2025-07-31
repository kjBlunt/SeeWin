#include "include/app_delegate.h"
#include "include/macros.h"
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

static NSUInteger applicationShouldTerminate() {
    printf("Application termination requested\n");
    terminated = true;
    return 0;
}

id setupAppDelegate() {
    Class NSObjectClass = objc_getClass("NSObject");

    Class AppDelegateClass = objc_allocateClassPair(NSObjectClass, "AppDelegate", 0);
    assert(AppDelegateClass != Nil);

    Protocol* NSApplicationDelegate = objc_getProtocol("NSApplicationDelegate");
    bool didAddProtocol = class_addProtocol(AppDelegateClass, NSApplicationDelegate);
    assert(didAddProtocol);

    SEL selTerminate = SEL("applicationShouldTerminate:");
    bool didAddMethod = class_addMethod(
        AppDelegateClass,
        selTerminate,
        (IMP)applicationShouldTerminate,
        NSUIntegerEncoding "@:@"
    );
    assert(didAddMethod);

    objc_registerClassPair(AppDelegateClass);

    // Create and return an instance
    SEL allocSel = SEL_ALLOC;
    SEL initSel = SEL_INIT;
    id delegate = OBJC_CLASS_CALL_ID(AppDelegateClass, allocSel);
    delegate = OBJC_CALL_ID(delegate, initSel);

    return delegate;
}
