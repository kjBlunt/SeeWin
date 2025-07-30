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

extern id NSApp;
extern id const NSDefaultRunLoopMode;
bool terminated = false;

typedef CGRect NSRect;

int main()
{
    SEL allocSel = sel_registerName("alloc");
    SEL initSel = sel_registerName("init");

    #ifdef ARC_AVAILABLE
    @autoreleasepool
    {
    #else
    Class NSAutoreleasePoolClass = objc_getClass("NSAutoreleasePool");
    id poolAlloc = ((id (*)(Class, SEL))objc_msgSend)(NSAutoreleasePoolClass, allocSel);
    id pool = ((id (*)(id, SEL))objc_msgSend)(poolAlloc, initSel);
    #endif

    Class NSApplicationClass = objc_getClass("NSApplication");
    SEL sharedApplicationSel = sel_registerName("sharedApplication");
    ((id (*)(Class, SEL))objc_msgSend)(NSApplicationClass, sharedApplicationSel);

    SEL setActivationPolicySel = sel_registerName("setActivationPolicy:");
    ((void (*)(id, SEL, NSInteger))objc_msgSend)(NSApp, setActivationPolicySel, 1);

    id appDelegate = setupAppDelegate();
    ((void (*)(id, SEL, id))objc_msgSend)(NSApp, sel_registerName("setDelegate:"), appDelegate);

    id windowDelegate = setupWindowDelegate();
    createSwitcherWindow(windowDelegate);

    registerGlobalHotkey();

    Class NSDateClass = objc_getClass("NSDate");
    SEL distantPastSel = sel_registerName("distantPast");
    SEL nextEventMatchingMaskSel = sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:");
    SEL sendEventSel = sel_registerName("sendEvent:");
    SEL updateWindowsSel = sel_registerName("updateWindows");

    while(!terminated)
    {
        id distantPast = ((id (*)(Class, SEL))objc_msgSend)(NSDateClass, distantPastSel);
        id event = ((id (*)(id, SEL, NSUInteger, id, id, BOOL))objc_msgSend)
                   (NSApp, nextEventMatchingMaskSel, NSUIntegerMax, distantPast, NSDefaultRunLoopMode, YES);

        if(event)
        {
            ((void (*)(id, SEL, id))objc_msgSend)(NSApp, sendEventSel, event);
            
            if(terminated) break;
            
            ((void (*)(id, SEL))objc_msgSend)(NSApp, updateWindowsSel);
        }
        
        usleep(1000);
    }

    printf("Window switcher terminated\n");

    #ifdef ARC_AVAILABLE
    }
    #else
    ((void (*)(id, SEL))objc_msgSend)(pool, sel_registerName("drain"));
    #endif
    
    return 0;
}
