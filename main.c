#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <objc/message.h>
#include <objc/NSObjCRuntime.h>
#include <Carbon/Carbon.h>

#if __LP64__ || (TARGET_OS_EMBEDDED && !TARGET_OS_IPHONE) || TARGET_OS_WIN32 || NS_BUILD_32_LIKE_64
#define NSIntegerEncoding "q"
#define NSUIntegerEncoding "L"
#else
#define NSIntegerEncoding "i"
#define NSUIntegerEncoding "I"
#endif

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
#include <CoreGraphics/CGBase.h>
#include <CoreGraphics/CGGeometry.h>
typedef CGPoint NSPoint;
typedef CGRect NSRect;

#ifndef NSWindowCollectionBehaviorMoveToActiveSpace
#define NSWindowCollectionBehaviorMoveToActiveSpace (1 << 1)
#endif

extern id NSApp;
extern id const NSDefaultRunLoopMode;
#endif

#if defined(__OBJC__) && __has_feature(objc_arc)
#define ARC_AVAILABLE
#endif

// ABI handling for different architectures
#ifdef __arm64__
#define abi_objc_msgSend_stret objc_msgSend
#else
#define abi_objc_msgSend_stret objc_msgSend_stret
#endif

bool terminated = false;
id window = nil;

// App delegate to handle termination
NSUInteger applicationShouldTerminate(id self, SEL _sel, id sender)
{
    printf("Application termination requested\n");
    terminated = true;
    return 0; // NSTerminateCancel
}

// Window delegate to handle window close
void windowWillClose(id self, SEL _sel, id notification)
{
    printf("Window will close\n");
    terminated = true;
}

void toggleWindowVisibility()
{
    SEL isVisibleSel = sel_registerName("isVisible");
    BOOL visible = ((BOOL (*)(id, SEL))objc_msgSend)(window, isVisibleSel);

    if (visible) {
        SEL orderOutSel = sel_registerName("orderOut:");
        ((void (*)(id, SEL, id))objc_msgSend)(window, orderOutSel, window);
    } else {
        SEL makeKeyAndOrderFrontSel = sel_registerName("makeKeyAndOrderFront:");
        ((void (*)(id, SEL, id))objc_msgSend)(window, makeKeyAndOrderFrontSel, window);

        SEL activateIgnoringOtherAppsSel = sel_registerName("activateIgnoringOtherApps:");
        ((void (*)(id, SEL, BOOL))objc_msgSend)(NSApp, activateIgnoringOtherAppsSel, YES);
    }
}

OSStatus HotKeyHandler(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData)
{
    EventHotKeyID hkCom;
    GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(hkCom), NULL, &hkCom);

    if (hkCom.signature == 'swch' && hkCom.id == 1) {
        SEL isVisibleSel = sel_registerName("isVisible");
        BOOL visible = ((BOOL (*)(id, SEL))objc_msgSend)(window, isVisibleSel);

        toggleWindowVisibility();
    }

    return noErr;
}

void customKeyDown(id self, SEL _cmd, id event)
{
    SEL charactersIgnoringModifiersSel = sel_registerName("charactersIgnoringModifiers");
    id chars = ((id (*)(id, SEL))objc_msgSend)(event, charactersIgnoringModifiersSel);

    SEL isEqualToStringSel = sel_registerName("isEqualToString:");
    id qString = ((id (*)(Class, SEL, const char*))objc_msgSend)(
        objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), "q"
    );

    BOOL isQ = ((BOOL (*)(id, SEL, id))objc_msgSend)(chars, isEqualToStringSel, qString);

    if (isQ) {
        ((void (*)(id, SEL, id))objc_msgSend)(self, sel_registerName("orderOut:"), self);
        return;
    }

    struct objc_super superInfo = {
        .receiver = self,
        .super_class = class_getSuperclass(object_getClass(self))
    };
    ((void (*)(struct objc_super*, SEL, id))objc_msgSendSuper)(&superInfo, _cmd, event);
}

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

    Class NSWindowClass = objc_getClass("NSWindow");

    // Create a subclass called "SwitcherWindow"
    Class SwitcherWindowClass = objc_allocateClassPair(NSWindowClass, "SwitcherWindow", 0);
    assert(SwitcherWindowClass != Nil);

    // Add keyDown: method
    BOOL didAddKeyDown = class_addMethod(
        SwitcherWindowClass,
        sel_registerName("keyDown:"),
        (IMP)customKeyDown,
        "v@:@"
    );
    assert(didAddKeyDown);

    // Register the class
    objc_registerClassPair(SwitcherWindowClass);

    // Initialize NSApplication
    Class NSApplicationClass = objc_getClass("NSApplication");
    SEL sharedApplicationSel = sel_registerName("sharedApplication");
    ((id (*)(Class, SEL))objc_msgSend)(NSApplicationClass, sharedApplicationSel);

    // 0 = Regular App (dok, menubar etc.), 1 = Accessory (window but no dock etc.), 2 = Prohibited (no windows, no docks etc.)
    SEL setActivationPolicySel = sel_registerName("setActivationPolicy:");
    ((void (*)(id, SEL, NSInteger))objc_msgSend)(NSApp, setActivationPolicySel, 1);

    // Create app delegate
    Class NSObjectClass = objc_getClass("NSObject");
    Class AppDelegateClass = objc_allocateClassPair(NSObjectClass, "AppDelegate", 0);
    Protocol* NSApplicationDelegateProtocol = objc_getProtocol("NSApplicationDelegate");
    bool resultAddProtoc = class_addProtocol(AppDelegateClass, NSApplicationDelegateProtocol);
    assert(resultAddProtoc);
    
    SEL applicationShouldTerminateSel = sel_registerName("applicationShouldTerminate:");
    bool resultAddMethod = class_addMethod(AppDelegateClass, applicationShouldTerminateSel, 
                                         (IMP)applicationShouldTerminate, NSUIntegerEncoding "@:@");
    assert(resultAddMethod);
    
    id dgAlloc = ((id (*)(Class, SEL))objc_msgSend)(AppDelegateClass, allocSel);
    id dg = ((id (*)(id, SEL))objc_msgSend)(dgAlloc, initSel);

    #ifndef ARC_AVAILABLE
    SEL autoreleaseSel = sel_registerName("autorelease");
    ((void (*)(id, SEL))objc_msgSend)(dg, autoreleaseSel);
    #endif

    // Set app delegate
    SEL setDelegateSel = sel_registerName("setDelegate:");
    ((void (*)(id, SEL, id))objc_msgSend)(NSApp, setDelegateSel, dg);

    // Finish launching
    SEL finishLaunchingSel = sel_registerName("finishLaunching");
    ((void (*)(id, SEL))objc_msgSend)(NSApp, finishLaunchingSel);

    EventHotKeyRef gMyHotKeyRef;
    EventHotKeyID gMyHotKeyID;
    EventTypeSpec eventType;

    gMyHotKeyID.signature = 'swch';
    gMyHotKeyID.id = 1;

    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;

    InstallApplicationEventHandler(&HotKeyHandler, 1, &eventType, NULL, NULL);

    RegisterEventHotKey(kVK_Space, controlKey, gMyHotKeyID, GetApplicationEventTarget(), 0, &gMyHotKeyRef);


    // Create the window (smaller size for a switcher)
    NSRect rect = {{100, 100}, {400, 300}};
    id windowAlloc = ((id (*)(Class, SEL))objc_msgSend)(SwitcherWindowClass, allocSel);
    SEL initWithContentRectSel = sel_registerName("initWithContentRect:styleMask:backing:defer:");

    // ToDo: figure out how to keep all but the menubar and traffic lights (maybe liquid glass hmmge)
    window = ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)
                (windowAlloc, initWithContentRectSel, rect, 0, 2, NO);
    
    #ifndef ARC_AVAILABLE
    ((void (*)(id, SEL))objc_msgSend)(window, autoreleaseSel);
    #endif

    // Don't release when closed (for manual memory management)
    SEL setReleasedWhenClosedSel = sel_registerName("setReleasedWhenClosed:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(window, setReleasedWhenClosedSel, NO);

    // Create window delegate
    Class WindowDelegateClass = objc_allocateClassPair(NSObjectClass, "WindowDelegate", 0);
    Protocol* NSWindowDelegateProtocol = objc_getProtocol("NSWindowDelegate");
    resultAddProtoc = class_addProtocol(WindowDelegateClass, NSWindowDelegateProtocol);
    assert(resultAddProtoc);
    
    SEL windowWillCloseSel = sel_registerName("windowWillClose:");
    resultAddMethod = class_addMethod(WindowDelegateClass, windowWillCloseSel, 
                                    (IMP)windowWillClose, "v@:@");
    assert(resultAddMethod);
    
    id wdgAlloc = ((id (*)(Class, SEL))objc_msgSend)(WindowDelegateClass, allocSel);
    id wdg = ((id (*)(id, SEL))objc_msgSend)(wdgAlloc, initSel);
    
    #ifndef ARC_AVAILABLE
    ((void (*)(id, SEL))objc_msgSend)(wdg, autoreleaseSel);
    #endif

    // Set window delegate
    ((void (*)(id, SEL, id))objc_msgSend)(window, setDelegateSel, wdg);

    // Set window title
    Class NSStringClass = objc_getClass("NSString");
    SEL stringWithUTF8StringSel = sel_registerName("stringWithUTF8String:");
    id titleString = ((id (*)(Class, SEL, const char*))objc_msgSend)
                     (NSStringClass, stringWithUTF8StringSel, "Window Switcher");
    SEL setTitleSel = sel_registerName("setTitle:");
    ((void (*)(id, SEL, id))objc_msgSend)(window, setTitleSel, titleString);

    // Get the content view
    SEL contentViewSel = sel_registerName("contentView");
    id contentView = ((id (*)(id, SEL))objc_msgSend)(window, contentViewSel);

    // ToDo: figure out why this doesnt do shit
    SEL setMovableByWindowBackgroundSel = sel_registerName("setMovableByWindowBackground:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(window, setMovableByWindowBackgroundSel, YES);

    // Center the window on screen
    SEL centerSel = sel_registerName("center");
    ((void (*)(id, SEL))objc_msgSend)(window, centerSel);

    // Create a scroll view to contain the app list
    Class NSScrollViewClass = objc_getClass("NSScrollView");
    id scrollViewAlloc = ((id (*)(Class, SEL))objc_msgSend)(NSScrollViewClass, allocSel);
    id scrollView = ((id (*)(id, SEL))objc_msgSend)(scrollViewAlloc, initSel);
    
    #ifndef ARC_AVAILABLE
    ((void (*)(id, SEL))objc_msgSend)(scrollView, autoreleaseSel);
    #endif

    // Set scroll view frame to fill the window
    SEL setFrameSel = sel_registerName("setFrame:");
    NSRect scrollFrame = {{0, 0}, {400, 300}};
    ((void (*)(id, SEL, NSRect))objc_msgSend)(scrollView, setFrameSel, scrollFrame);

    // Enable vertical scrolling
    SEL setHasVerticalScrollerSel = sel_registerName("setHasVerticalScroller:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(scrollView, setHasVerticalScrollerSel, YES);

    // Create a text view to display the application list
    Class NSTextViewClass = objc_getClass("NSTextView");
    id textViewAlloc = ((id (*)(Class, SEL))objc_msgSend)(NSTextViewClass, allocSel);
    SEL initWithFrameSel = sel_registerName("initWithFrame:");
    NSRect textFrame = {{0, 0}, {380, 300}};
    id textView = ((id (*)(id, SEL, NSRect))objc_msgSend)(textViewAlloc, initWithFrameSel, textFrame);
    
    #ifndef ARC_AVAILABLE
    ((void (*)(id, SEL))objc_msgSend)(textView, autoreleaseSel);
    #endif

    // Make text view non-editable and non-selectable
    SEL setEditableSel = sel_registerName("setEditable:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(textView, setEditableSel, NO);
    
    SEL setSelectableSel = sel_registerName("setSelectable:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(textView, setSelectableSel, NO);

    // Set the text view as the document view of the scroll view
    SEL setDocumentViewSel = sel_registerName("setDocumentView:");
    ((void (*)(id, SEL, id))objc_msgSend)(scrollView, setDocumentViewSel, textView);

    // Add scroll view to the content view
    SEL addSubviewSel = sel_registerName("addSubview:");
    ((void (*)(id, SEL, id))objc_msgSend)(contentView, addSubviewSel, scrollView);

    // Get running applications using NSWorkspace
    Class NSWorkspaceClass = objc_getClass("NSWorkspace");
    SEL sharedWorkspaceSel = sel_registerName("sharedWorkspace");
    id workspace = ((id (*)(Class, SEL))objc_msgSend)(NSWorkspaceClass, sharedWorkspaceSel);
    
    SEL runningApplicationsSel = sel_registerName("runningApplications");
    id runningApps = ((id (*)(id, SEL))objc_msgSend)(workspace, runningApplicationsSel);

    // Create a mutable string to build our app list
    Class NSMutableStringClass = objc_getClass("NSMutableString");
    id appListString = ((id (*)(Class, SEL))objc_msgSend)(NSMutableStringClass, allocSel);
    SEL initWithCapacitySel = sel_registerName("initWithCapacity:");
    appListString = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(appListString, initWithCapacitySel, 1000);
    
    #ifndef ARC_AVAILABLE
    ((void (*)(id, SEL))objc_msgSend)(appListString, autoreleaseSel);
    #endif

    // Add header
    id headerString = ((id (*)(Class, SEL, const char*))objc_msgSend)
                      (NSStringClass, stringWithUTF8StringSel, "Running Applications:\n\n");
    SEL appendStringSel = sel_registerName("appendString:");
    ((void (*)(id, SEL, id))objc_msgSend)(appListString, appendStringSel, headerString);

    // Iterate through running applications
    SEL countSel = sel_registerName("count");
    NSUInteger appCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(runningApps, countSel);
    
    SEL objectAtIndexSel = sel_registerName("objectAtIndex:");
    SEL localizedNameSel = sel_registerName("localizedName");
    SEL bundleIdentifierSel = sel_registerName("bundleIdentifier");
    SEL isActiveSel = sel_registerName("isActive");
    SEL UTF8StringSel = sel_registerName("UTF8String");

    SEL isHiddenSel = sel_registerName("isHidden");
    SEL activationPolicySel = sel_registerName("activationPolicy");

    for (NSUInteger i = 0; i < appCount; i++) {
        id app = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(runningApps, objectAtIndexSel, i);

        // Skip background-only or hidden apps
        BOOL isHidden = ((BOOL (*)(id, SEL))objc_msgSend)(app, isHiddenSel);
        NSInteger policy = ((NSInteger (*)(id, SEL))objc_msgSend)(app, activationPolicySel);
        
        // Policy: 0 = Regular UI app, 1 = Accessory (no Dock), 2 = Background
        if (isHidden || policy != 0) continue;
        
        // Get app name
        id appName = ((id (*)(id, SEL))objc_msgSend)(app, localizedNameSel);
        if(!appName) continue;
        
        // Get bundle identifier (optional, for debugging)
        id bundleId = ((id (*)(id, SEL))objc_msgSend)(app, bundleIdentifierSel);
        
        // Check if app is active
        BOOL isActive = ((BOOL (*)(id, SEL))objc_msgSend)(app, isActiveSel);
        
        // Convert to C string
        const char* appNameCStr = ((const char* (*)(id, SEL))objc_msgSend)(appName, UTF8StringSel);
        
        // Create formatted string
        char buffer[512];
        if(isActive) {
            snprintf(buffer, sizeof(buffer), "[ACTIVE] %s\n", appNameCStr);
        } else {
            snprintf(buffer, sizeof(buffer), "%s\n", appNameCStr);
        }
        
        id lineString = ((id (*)(Class, SEL, const char*))objc_msgSend)
                        (NSStringClass, stringWithUTF8StringSel, buffer);
        ((void (*)(id, SEL, id))objc_msgSend)(appListString, appendStringSel, lineString);
    }

    // Set the text in the text view
    SEL setStringSel = sel_registerName("setString:");
    ((void (*)(id, SEL, id))objc_msgSend)(textView, setStringSel, appListString);


    // Set window background color
    Class NSColorClass = objc_getClass("NSColor");
    id backgroundColor = ((id (*)(Class, SEL))objc_msgSend)(NSColorClass, sel_registerName("windowBackgroundColor"));
    SEL setBackgroundColorSel = sel_registerName("setBackgroundColor:");
    ((void (*)(id, SEL, id))objc_msgSend)(window, setBackgroundColorSel, backgroundColor);

    NSUInteger behavior = NSWindowCollectionBehaviorMoveToActiveSpace;
    SEL setCollectionBehaviorSel = sel_registerName("setCollectionBehavior:");
    ((void (*)(id, SEL, NSUInteger))objc_msgSend)(window, setCollectionBehaviorSel, behavior);

    // Show the window
    SEL makeKeyAndOrderFrontSel = sel_registerName("makeKeyAndOrderFront:");
    ((void (*)(id, SEL, id))objc_msgSend)(window, makeKeyAndOrderFrontSel, window);

    // Activate the app
    SEL activateIgnoringOtherAppsSel = sel_registerName("activateIgnoringOtherApps:");
    ((void (*)(id, SEL, BOOL))objc_msgSend)(NSApp, activateIgnoringOtherAppsSel, YES);

    printf("Window switcher started\n");

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
