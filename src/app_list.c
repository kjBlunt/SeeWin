#include "include/app_list.h"
#include "include/switcher_window.h"
#include "include/macros.h"
#include "include/label_prefixes.h"
#include <objc/runtime.h>
#include <objc/message.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <objc/NSObjCRuntime.h>
#include <objc/objc.h>
#include <CoreFoundation/CFCGTypes.h>


extern id appButtons;
extern id favoriteButtons;
extern NSUInteger selectedIndex;
extern NSUInteger totalFavorites;

static void addSeparatorToStackView(id stackView)
{
    Class NSView = objc_getClass("NSView");
    id separator = OBJC_CALL_ID(OBJC_CLASS_CALL_ID(NSView, SEL_ALLOC), SEL_INIT);

    SEL setTranslatesAutoresizingMaskIntoConstraintsSel = SEL("setTranslatesAutoresizingMaskIntoConstraints:");
    OBJC_CALL_VOID_BOOL(separator, setTranslatesAutoresizingMaskIntoConstraintsSel, NO);

    id heightAnchor = OBJC_CALL_ID(separator, SEL("heightAnchor"));
    id heightConstraint = OBJC_CALL_FLOAT(id, heightAnchor, SEL("constraintEqualToConstant:"), 1.0);
    OBJC_CALL_VOID_BOOL(heightConstraint, SEL("setActive:"), YES);

    OBJC_CALL_VOID_BOOL(separator, SEL("setWantsLayer:"), YES);
    id layer = OBJC_CALL_ID(separator, SEL("layer"));
    
    id grayColor = OBJC_CLASS_CALL_ID(objc_getClass("NSColor"), SEL("grayColor"));
    id cgColor = OBJC_CALL_ID(grayColor, SEL("CGColor"));
    OBJC_CALL_VOID_ARG(layer, SEL("setBackgroundColor:"), cgColor);

    SEL addArrangedSubviewSel = SEL("addArrangedSubview:");
    OBJC_CALL_VOID_ARG(stackView, addArrangedSubviewSel, separator);
}

void updateAppList(id stackView)
{
    SEL arrangedSubviewsSel = SEL("arrangedSubviews");
    id subviews = OBJC_CALL_ID(stackView, arrangedSubviewsSel);

    SEL countSel = SEL_COUNT;
    SEL objectAtIndexSel = SEL_OBJECT_AT_INDEX;
    SEL removeArrangedSubviewSel = SEL("removeArrangedSubview:");
    SEL removeFromSuperviewSel = SEL("removeFromSuperview");

    NSUInteger count = OBJC_CALL_UINT(subviews, countSel);
    for (NSUInteger i = 0; i < count; ++i) {
        id subview = OBJC_CALL_ID_UINT(subviews, objectAtIndexSel, i);
        OBJC_CALL_VOID_ARG(stackView, removeArrangedSubviewSel, subview);
        OBJC_CALL_VOID(subview, removeFromSuperviewSel);
    }

    if (!appButtons) {
        appButtons = OBJC_CLASS_CALL_ID(objc_getClass("NSMutableArray"), SEL_ALLOC);
        appButtons = OBJC_CALL_ID(appButtons, SEL_INIT);
    } else {
        OBJC_CALL_VOID(appButtons, SEL("removeAllObjects"));
    }

    selectedIndex = 0;
    totalFavorites = 0;

    // Add favorites first if they exist
    if (favoriteButtons) {
        NSUInteger favCount = OBJC_CALL_UINT(favoriteButtons, countSel);
        totalFavorites = favCount;
        
        if (favCount > 0) {
            for (NSUInteger i = 0; i < favCount; ++i) {
                id favButton = OBJC_CALL_ID_UINT(favoriteButtons, objectAtIndexSel, i);
                SEL addArrangedSubviewSel = SEL("addArrangedSubview:");
                OBJC_CALL_VOID_ARG(stackView, addArrangedSubviewSel, favButton);
            }
            
            addSeparatorToStackView(stackView);
        }
    }

    Class NSWorkspace = objc_getClass("NSWorkspace");
    id workspace = OBJC_CLASS_CALL_ID(NSWorkspace, SEL("sharedWorkspace"));
    id apps = OBJC_CALL_ID(workspace, SEL("runningApplications"));

    SEL nameSel = SEL("localizedName");
    SEL isHiddenSel = SEL("isHidden");
    SEL activationPolicySel = SEL("activationPolicy");
    SEL isActiveSel = SEL("isActive");

    Class NSButton = objc_getClass("NSButton");
    Class NSString = objc_getClass("NSString");

    SEL allocSel = SEL_ALLOC;
    SEL initSel = SEL_INIT;
    SEL setTitleSel = SEL_SET_TITLE;
    SEL stringWithUTF8Sel = SEL_STRING_WITH_UTF8_STRING;
    SEL addArrangedSubviewSel = SEL("addArrangedSubview:");

    SEL appsCountSel = SEL_COUNT;
    NSUInteger appCount = OBJC_CALL_UINT(apps, appsCountSel);

    for (NSUInteger i = 0; i < appCount; ++i) {
        id app = OBJC_CALL_ID_UINT(apps, objectAtIndexSel, i);

        BOOL isHidden = OBJC_CALL_BOOL(app, isHiddenSel);
        NSInteger policy = OBJC_CALL(NSInteger, app, activationPolicySel);
        if (isHidden || policy != 0) continue;

        id name = OBJC_CALL_ID(app, nameSel);
        if (!name) continue;

        const char* cname = OBJC_CALL_CSTRING(name, SEL_UTF8_STRING);
        
        BOOL isInFavorites = NO;
        if (favoriteButtons) {
            NSUInteger favCount = OBJC_CALL_UINT(favoriteButtons, countSel);
            for (NSUInteger j = 0; j < favCount; ++j) {
                id favButton = OBJC_CALL_ID_UINT(favoriteButtons, objectAtIndexSel, j);
                id favTitle = OBJC_CALL_ID(favButton, SEL_TITLE);
                const char* favTitleStr = OBJC_CALL_CSTRING(favTitle, SEL_UTF8_STRING);
                Class NSFont = objc_getClass("NSFont");
                SEL monoSel = SEL("monospacedSystemFontOfSize:weight:");
                id font = OBJC_CLASS_CALL_FLOAT_INT(id, NSFont, monoSel, 15.0, 5);
                OBJC_CALL_VOID_ARG(favButton, SEL("setFont:"), font);
                
                const char* favAppName = strip_prefix(favTitleStr);
                
                if (strcmp(cname, favAppName) == 0) {
                    isInFavorites = YES;
                    break;
                }
            }
        }
        
        if (isInFavorites) continue;

        BOOL isActive = OBJC_CALL_BOOL(app, isActiveSel);

        char label[256];
        snprintf(label, sizeof(label), "%s%s", isActive ? PREFIX_ACTIVE : "", cname);

        id titleString = OBJC_CLASS_CALL_ID_CSTRING(NSString, stringWithUTF8Sel, label);
        id button = OBJC_CALL_ID(OBJC_CLASS_CALL_ID(NSButton, allocSel), initSel);

        SEL setButtonTypeSel = SEL("setButtonType:");
        OBJC_CALL_VOID_UINT(button, setButtonTypeSel, 0); // NSButtonTypeMomentaryChange = 0

        SEL setBorderedSel = SEL("setBordered:");
        OBJC_CALL_VOID_BOOL(button, setBorderedSel, NO);

        SEL setBezelStyleSel = SEL("setBezelStyle:");
        OBJC_CALL_VOID_UINT(button, setBezelStyleSel, 15); // NSBezelStyleRegularSquare = 15

        SEL setFocusRingTypeSel = SEL("setFocusRingType:");
        OBJC_CALL_VOID_UINT(button, setFocusRingTypeSel, 1); // NSFocusRingTypeNone = 1

        OBJC_CALL_VOID_ARG(button, setTitleSel, titleString);
        Class NSFont = objc_getClass("NSFont");
        SEL monoSel = SEL("monospacedSystemFontOfSize:weight:");
        id font = OBJC_CLASS_CALL_FLOAT_INT(id, NSFont, monoSel, 15.0, 5);
        OBJC_CALL_VOID_ARG(button, SEL("setFont:"), font);
        OBJC_CALL_VOID_ARG(stackView, addArrangedSubviewSel, button);

        // Add to appButtons array
        OBJC_CALL_VOID_ARG(appButtons, SEL_ADD_OBJECT, button);
        SEL setRepresentedObjectSel = SEL_SET_REPRESENTED_OBJECT;
        OBJC_CALL_VOID_ARG(button, setRepresentedObjectSel, app);
    }

    updateSelectionHighlight();
}
