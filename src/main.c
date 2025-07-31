#include <CoreFoundation/CFCGTypes.h>
#include <stdio.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <unistd.h>

#include "include/hotkey.h"
#include "include/switcher_window.h"
#include "include/app_delegate.h"
#include "include/window_delegate.h"
#include "include/macros.h"
#include "include/config.h"

extern id NSApp;
extern id const NSDefaultRunLoopMode;
bool terminated = false;

typedef CGRect NSRect;

int main()
{
    load_config();

    #ifdef ARC_AVAILABLE
    @autoreleasepool
    {
    #else
    id pool = OBJC_CLASS_CALL_ID(objc_getClass("NSAutoreleasePool"), SEL_ALLOC);
    pool = OBJC_CALL_ID(pool, SEL_INIT);
    #endif

    OBJC_CLASS_CALL_ID(objc_getClass("NSApplication"), SEL("sharedApplication"));
    OBJC_CALL_VOID_INT(NSApp, SEL("setActivationPolicy:"), 1);

    id appDelegate = setupAppDelegate();
    OBJC_CALL_VOID_ARG(NSApp, SEL("setDelegate:"), appDelegate);

    id windowDelegate = setupWindowDelegate();
    createSwitcherWindow(windowDelegate);
    registerGlobalHotkey();

    Class NSDateClass = objc_getClass("NSDate");
    while (!terminated)
    {
        id distantPast = OBJC_CLASS_CALL_ID(NSDateClass, SEL("distantPast"));
        id event = OBJC_CALL_ID_EVENT(NSApp, SEL("nextEventMatchingMask:untilDate:inMode:dequeue:"),
                                NSUIntegerMax, distantPast, NSDefaultRunLoopMode, YES);

        if (event)
        {
            OBJC_CALL_VOID_ARG(NSApp, SEL("sendEvent:"), event);
            if (terminated) break;
            OBJC_CALL_VOID(NSApp, SEL("updateWindows"));
        }
        usleep(1000);
    }
    printf("Window switcher terminated\n");

    #ifdef ARC_AVAILABLE
    }
    #else
    OBJC_CALL_VOID(pool, SEL("drain"));
    #endif

    return 0;
}

