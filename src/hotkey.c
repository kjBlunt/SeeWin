#include "include/hotkey.h"
#include "include/macros.h"
#include "include/config.h"
#include <Carbon/Carbon.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <assert.h>

extern id window;
extern id NSApp;

OSStatus HotKeyHandler(EventHandlerCallRef nextHandler, EventRef theEvent, void *userData) {
    (void)userData;
    (void)nextHandler;
    EventHotKeyID hkCom;
    GetEventParameter(theEvent, kEventParamDirectObject, typeEventHotKeyID, NULL, sizeof(hkCom), NULL, &hkCom);

    if (hkCom.signature == 'swch' && hkCom.id == 1) {
        SEL toggleSel = SEL("toggleVisibility");
        OBJC_CALL_VOID(window, toggleSel);
    }

    return noErr;
}

void registerGlobalHotkey() {
    load_config();

    EventHotKeyRef gHotKeyRef;
    EventHotKeyID gHotKeyID;
    EventTypeSpec eventType;

    gHotKeyID.signature = 'swch';
    gHotKeyID.id = 1;

    eventType.eventClass = kEventClassKeyboard;
    eventType.eventKind = kEventHotKeyPressed;

    InstallApplicationEventHandler(&HotKeyHandler, 1, &eventType, NULL, NULL);
    RegisterEventHotKey(config.hotkey_keycode, config.hotkey_modifier, gHotKeyID, GetApplicationEventTarget(), 0, &gHotKeyRef);
}

