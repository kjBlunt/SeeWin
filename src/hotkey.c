#include "include/hotkey.h"
#include "include/macros.h"
#include <Carbon/Carbon.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <assert.h>

extern id window;
extern id NSApp;

OSStatus HotKeyHandler(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData)
{
    EventHotKeyID hkCom;
    GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(hkCom), NULL, &hkCom);

    if (hkCom.signature == 'swch' && hkCom.id == 1) {
        SEL isVisibleSel = SEL("isVisible");

        SEL toggleSel = SEL("toggleVisibility");
        OBJC_CALL_VOID(window, toggleSel);
    }

    return noErr;
}

void registerGlobalHotkey()
{
    EventHotKeyRef gHotKeyRef;
    EventHotKeyID gHotKeyID;
    EventTypeSpec eventType;

    gHotKeyID.signature = 'swch';
    gHotKeyID.id = 1;

    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;

    InstallApplicationEventHandler(&HotKeyHandler, 1, &eventType, NULL, NULL);
    RegisterEventHotKey(kVK_Space, cmdKey, gHotKeyID, GetApplicationEventTarget(), 0, &gHotKeyRef);
}
