#define SEL(NAME) sel_registerName(NAME)

#ifndef OBJC_MACROS_H
#define OBJC_MACROS_H

#include <objc/runtime.h>
#include <objc/message.h>
#include <CoreGraphics/CGGeometry.h>

// Basic void return macros
#define OBJC_CALL_VOID(obj, sel) \
    ((void (*)(id, SEL))objc_msgSend)(obj, sel)

#define OBJC_CALL_VOID_ARG(obj, sel, arg) \
    ((void (*)(id, SEL, id))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_BOOL(obj, sel, arg) \
    ((void (*)(id, SEL, BOOL))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_INT(obj, sel, arg) \
    ((void (*)(id, SEL, NSInteger))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_UINT(obj, sel, arg) \
    ((void (*)(id, SEL, NSUInteger))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_FLOAT(obj, sel, arg) \
    ((void (*)(id, SEL, CGFloat))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_DOUBLE(obj, sel, arg) \
    ((void (*)(id, SEL, double))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_RECT(obj, sel, arg) \
    ((void (*)(id, SEL, NSRect))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_SIZE(obj, sel, arg) \
    ((void (*)(id, SEL, NSSize))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_VOID_FLOAT_UINT(obj, sel, arg1, arg2) \
    ((void (*)(id, SEL, float, NSUInteger))objc_msgSend)(obj, sel, arg1, arg2)

// Macros for specific return types
#define OBJC_CALL_ID(obj, sel) \
    ((id (*)(id, SEL))objc_msgSend)(obj, sel)

#define OBJC_CALL_ID_UINT(obj, sel, arg) \
    ((id (*)(id, SEL, NSUInteger))objc_msgSend)(obj, sel, arg)

#define OBJC_CALL_ID_RECT_UINT_UINT_BOOL(obj, sel, rect, mask, backing, defer) \
    ((id (*)(id, SEL, NSRect, NSUInteger, NSUInteger, BOOL))objc_msgSend)(obj, sel, rect, mask, backing, defer)

#define OBJC_CALL_CSTRING(obj, sel) \
    ((const char* (*)(id, SEL))objc_msgSend)(obj, sel)

#define OBJC_CALL_NSSIZE(obj, sel) \
    ((NSSize (*)(id, SEL))objc_msgSend)(obj, sel)

#define OBJC_CALL_BOOL(obj, sel) \
    ((BOOL (*)(id, SEL))objc_msgSend)(obj, sel)

#define OBJC_CALL_UINT(obj, sel) \
    ((NSUInteger (*)(id, SEL))objc_msgSend)(obj, sel)

// Class method macros
#define OBJC_CLASS_CALL_ID(cls, sel) \
    ((id (*)(Class, SEL))objc_msgSend)(cls, sel)

#define OBJC_CLASS_CALL_ID_CSTRING(cls, sel, cstr) \
    ((id (*)(Class, SEL, const char*))objc_msgSend)(cls, sel, cstr)

// Generic macros with return type as parameter
#define OBJC_CALL(return_type, obj, sel) \
    ((return_type (*)(id, SEL))objc_msgSend)(obj, sel)

// Specific type macros to avoid typeof issues
#define OBJC_CALL_FLOAT(return_type, obj, sel, arg) \
    ((return_type (*)(id, SEL, CGFloat))objc_msgSend)(obj, sel, arg)

#define OBJC_CLASS_CALL(return_type, cls, sel) \
    ((return_type (*)(Class, SEL))objc_msgSend)(cls, sel)

#define OBJC_CLASS_CALL_FLOAT_INT(return_type, cls, sel, arg1, arg2) \
    ((return_type (*)(Class, SEL, CGFloat, NSInteger))objc_msgSend)(cls, sel, arg1, arg2)

// Super call macro
#define OBJC_SUPER_CALL_VOID_ARG(super_ptr, sel, arg) \
    ((void (*)(struct objc_super*, SEL, id))objc_msgSendSuper)(super_ptr, sel, arg)

// Event fetching
#define OBJC_CALL_ID_EVENT(obj, sel, mask, date, mode, dequeue) \
    ((id (*)(id, SEL, NSUInteger, id, id, BOOL))objc_msgSend)(obj, sel, mask, date, mode, dequeue)

// Convenience macros for common selectors
#define SEL_ALLOC SEL("alloc")
#define SEL_INIT SEL("init")
#define SEL_COUNT SEL("count")
#define SEL_OBJECT_AT_INDEX SEL("objectAtIndex:")
#define SEL_ADD_OBJECT SEL("addObject:")
#define SEL_REMOVE_OBJECT_AT_INDEX SEL("removeObjectAtIndex:")
#define SEL_UTF8_STRING SEL("UTF8String")
#define SEL_STRING_WITH_UTF8_STRING SEL("stringWithUTF8String:")
#define SEL_TITLE SEL("title")
#define SEL_SET_TITLE SEL("setTitle:")
#define SEL_REPRESENTED_OBJECT SEL("representedObject")
#define SEL_SET_REPRESENTED_OBJECT SEL("setRepresentedObject:")

#endif
