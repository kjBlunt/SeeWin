#include "include/app_list.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#define SEL(NAME) sel_registerName(NAME)

void updateAppList(id textView)
{
    Class NSWorkspace = objc_getClass("NSWorkspace");
    id workspace = ((id (*)(Class, SEL))objc_msgSend)(NSWorkspace, SEL("sharedWorkspace"));
    id runningApps = ((id (*)(id, SEL))objc_msgSend)(workspace, SEL("runningApplications"));

    Class NSMutableString = objc_getClass("NSMutableString");
    id appList = ((id (*)(Class, SEL))objc_msgSend)(NSMutableString, SEL("alloc"));
    appList = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(appList, SEL("initWithCapacity:"), 1000);

    Class NSString = objc_getClass("NSString");
    id header = ((id (*)(Class, SEL, const char*))objc_msgSend)(NSString, SEL("stringWithUTF8String:"), "Running Applications:\n\n");
    ((void (*)(id, SEL, id))objc_msgSend)(appList, SEL("appendString:"), header);

    NSUInteger count = ((NSUInteger (*)(id, SEL))objc_msgSend)(runningApps, SEL("count"));
    SEL objectAtIndexSel = SEL("objectAtIndex:");
    SEL nameSel = SEL("localizedName");
    SEL isHiddenSel = SEL("isHidden");
    SEL isActiveSel = SEL("isActive");
    SEL activationPolicySel = SEL("activationPolicy");
    SEL utf8Sel = SEL("UTF8String");

    for (NSUInteger i = 0; i < count; ++i) {
        id app = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(runningApps, objectAtIndexSel, i);

        BOOL isHidden = ((BOOL (*)(id, SEL))objc_msgSend)(app, isHiddenSel);
        NSInteger policy = ((NSInteger (*)(id, SEL))objc_msgSend)(app, activationPolicySel);
        if (isHidden || policy != 0) continue;

        id name = ((id (*)(id, SEL))objc_msgSend)(app, nameSel);
        if (!name) continue;

        BOOL isActive = ((BOOL (*)(id, SEL))objc_msgSend)(app, isActiveSel);
        const char* cname = ((const char* (*)(id, SEL))objc_msgSend)(name, utf8Sel);

        char buffer[512];
        snprintf(buffer, sizeof(buffer), isActive ? "[ACTIVE] %s\n" : "%s\n", cname);

        id line = ((id (*)(Class, SEL, const char*))objc_msgSend)(NSString, SEL("stringWithUTF8String:"), buffer);
        ((void (*)(id, SEL, id))objc_msgSend)(appList, SEL("appendString:"), line);
    }

    ((void (*)(id, SEL, id))objc_msgSend)(textView, SEL("setString:"), appList);
}

