#include "include/app_list.h"
#include "include/switcher_window.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <CoreFoundation/CFCGTypes.h>

#define SEL(NAME) sel_registerName(NAME)

extern id appButtons;
extern id favoriteButtons;
extern NSUInteger selectedIndex;
extern NSUInteger totalFavorites;

static void addSeparatorToStackView(id stackView)
{
    Class NSBox = objc_getClass("NSBox");
    id separator = ((id (*)(Class, SEL))objc_msgSend)(NSBox, sel_registerName("alloc"));
    separator = ((id (*)(id, SEL))objc_msgSend)(separator, sel_registerName("init"));
    
    // Configure as horizontal line separator
    SEL setBoxTypeSel = sel_registerName("setBoxType:");
    ((void (*)(id, SEL, NSUInteger))objc_msgSend)(separator, setBoxTypeSel, 2); // NSBoxSeparator
    
    // Set height constraint
    SEL widthAnchorSel = sel_registerName("widthAnchor");
    SEL heightAnchorSel = sel_registerName("heightAnchor");
    SEL constraintEqualToConstantSel = sel_registerName("constraintEqualToConstant:");
    SEL setActiveSel = sel_registerName("setActive:");
    
    id heightAnchor = ((id (*)(id, SEL))objc_msgSend)(separator, heightAnchorSel);
    id heightConstraint = ((id (*)(id, SEL, CGFloat))objc_msgSend)(heightAnchor, constraintEqualToConstantSel, 1.0);
    ((void (*)(id, SEL, BOOL))objc_msgSend)(heightConstraint, setActiveSel, YES);
    
    SEL addArrangedSubviewSel = SEL("addArrangedSubview:");
    ((void (*)(id, SEL, id))objc_msgSend)(stackView, addArrangedSubviewSel, separator);
}

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

    // Initialize appButtons array
    if (!appButtons) {
        appButtons = ((id (*)(Class, SEL))objc_msgSend)(objc_getClass("NSMutableArray"), sel_registerName("alloc"));
        appButtons = ((id (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("init"));
    } else {
        ((void (*)(id, SEL))objc_msgSend)(appButtons, sel_registerName("removeAllObjects"));
    }

    selectedIndex = 0;
    totalFavorites = 0;

    // Add favorites first if they exist
    if (favoriteButtons) {
        NSUInteger favCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(favoriteButtons, countSel);
        totalFavorites = favCount;
        
        if (favCount > 0) {
            // Add favorites section
            for (NSUInteger i = 0; i < favCount; ++i) {
                id favButton = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(favoriteButtons, objectAtIndexSel, i);
                SEL addArrangedSubviewSel = SEL("addArrangedSubview:");
                ((void (*)(id, SEL, id))objc_msgSend)(stackView, addArrangedSubviewSel, favButton);
            }
            
            // Add separator between favorites and regular apps
            addSeparatorToStackView(stackView);
        }
    }

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

        const char* cname = ((const char* (*)(id, SEL))objc_msgSend)(name, SEL("UTF8String"));
        
        // Check if this app is already in favorites
        BOOL isInFavorites = NO;
        if (favoriteButtons) {
            NSUInteger favCount = ((NSUInteger (*)(id, SEL))objc_msgSend)(favoriteButtons, countSel);
            for (NSUInteger j = 0; j < favCount; ++j) {
                id favButton = ((id (*)(id, SEL, NSUInteger))objc_msgSend)(favoriteButtons, objectAtIndexSel, j);
                id favTitle = ((id (*)(id, SEL))objc_msgSend)(favButton, sel_registerName("title"));
                const char* favTitleStr = ((const char* (*)(id, SEL))objc_msgSend)(favTitle, SEL("UTF8String"));
                
                // Skip the "★ " prefix when comparing
                const char* favAppName = favTitleStr;
                if (strncmp(favTitleStr, "★ ", 3) == 0) {
                    favAppName = favTitleStr + 3;
                }
                
                if (strcmp(cname, favAppName) == 0) {
                    isInFavorites = YES;
                    break;
                }
            }
        }
        
        // Skip apps that are already in favorites
        if (isInFavorites) continue;

        BOOL isActive = ((BOOL (*)(id, SEL))objc_msgSend)(app, isActiveSel);

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
