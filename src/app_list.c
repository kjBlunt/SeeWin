#include "include/app_list.h"
#include "include/switcher_window.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>

#define SEL(NAME) sel_registerName(NAME)

extern id appButtons;
extern NSUInteger selectedIndex;

void updateAppList(id stackView)
{
    printf("updateAppList\n");

    SEL arrangedSubviewsSel = SEL("arrangedSubviews");
    id subviews = ((id (*)(id, SEL))objc_msgSend)(stackView, arrangedSubviewsSel);

    SEL countSel = SEL("count");
    SEL objectAtIndexSel = SEL("objectAtIndex:");
    SEL removeArrangedSubviewSel = SEL("removeArrangedSubview:");
    SEL removeFromSuperviewSel = SEL("removeFromSuperview");

    NSUInteger count = ((NSUInteger (*)(id, SEL))objc_msgSend)(subviews, countSel);
    for (NSUInteger i = 0; i < count; ++i) {
        id subview = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(subviews, objectAtIndexSel, i);
        ((void (*)(id, SEL, id))objc_msgSend)(stackView, removeArrangedSubviewSel, subview);
        ((void (*)(id, SEL))objc_msgSend)(subview, removeFromSuperviewSel);
    }

    Class NSMutableArray = objc_getClass("NSMutableArray");
    appButtons = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSMutableArray"), sel_registerName("alloc"));
    appButtons = ((id (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("init"));
    selectedIndex = 0;

    Class NSWorkspace = objc_getClass("NSWorkspace");
    id workspace = ((id (*)(Class, SEL))objc_msgSend)(NSWorkspace, SEL("sharedWorkspace"));
    id apps = ((id (*)(id, SEL))objc_msgSend)(workspace, SEL("runningApplications"));

    SEL nameSel = SEL("localizedName");
    SEL isHiddenSel = SEL("isHidden");
    SEL activationPolicySel = SEL("activationPolicy");
    SEL isActiveSel = SEL("isActive");

    Class NSButton = objc_getClass("NSButton");
    Class NSString = objc_getClass("NSString");

    SEL allocSel = SEL("alloc");
    SEL initSel = SEL("init");
    SEL setTitleSel = SEL("setTitle:");
    SEL stringWithUTF8Sel = SEL("stringWithUTF8String:");
    SEL addArrangedSubviewSel = SEL("addArrangedSubview:");

    SEL appsCountSel = SEL("count");
    NSUInteger appCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(apps, appsCountSel);

    for (NSUInteger i = 0; i < appCount; ++i) {
        id app = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(apps, objectAtIndexSel, i);

        BOOL isHidden = ((BOOL (*)(id, SEL))objc_msgSend)(app, isHiddenSel);
        NSInteger policy = ((NSInteger (*)(id, SEL))objc_msgSend)(app, activationPolicySel);
        if (isHidden || policy != 0) continue;

        id name = ((id (*)(id, SEL))objc_msgSend)(app, nameSel);
        if (!name) continue;

        BOOL isActive = ((BOOL (*)(id, SEL))objc_msgSend)(app, isActiveSel);
        const char* cname = ((const char* (*)(id, SEL))objc_msgSend)(name, SEL("UTF8String"));

        char label[256];
        snprintf(label, sizeof(label), isActive ? "[*] %s" : "%s", cname);

        id titleString = ((id (*)(Class, SEL, const char*))objc_msgSend)(NSString, stringWithUTF8Sel, label);
        id button = ((id (*)(id, SEL))objc_msgSend)(
            ((id (*)(Class, SEL))objc_msgSend)(NSButton, allocSel),
            initSel
        );

        SEL setButtonTypeSel = sel_registerName("setButtonType:");
        ((void (*)(id, SEL, NSUInteger))objc_msgSend)(button, setButtonTypeSel, 0); // NSButtonTypeMomentaryChange = 0

        SEL setBorderedSel = sel_registerName("setBordered:");
        ((void (*)(id, SEL, BOOL))objc_msgSend)(button, setBorderedSel, NO);

        SEL setBezelStyleSel = sel_registerName("setBezelStyle:");
        ((void (*)(id, SEL, NSUInteger))objc_msgSend)(button, setBezelStyleSel, 15); // NSBezelStyleRegularSquare = 15

        SEL setFocusRingTypeSel = sel_registerName("setFocusRingType:");
        ((void (*)(id, SEL, NSUInteger))objc_msgSend)(button, setFocusRingTypeSel, 1); // NSFocusRingTypeNone = 1

        ((void (*)(id, SEL, id))objc_msgSend)(button, setTitleSel, titleString);
        ((void (*)(id, SEL, id))objc_msgSend)(stackView, addArrangedSubviewSel, button);

        // Add to appButtons array
        ((void (*)(id, SEL, id))objc_msgSend)(appButtons, SEL("addObject:"), button);
        SEL setRepresentedObjectSel = sel_registerName("setRepresentedObject:");
        ((void (*)(id, SEL, id))objc_msgSend)(button, setRepresentedObjectSel, app);
    }

    updateSelectionHighlight();
}
