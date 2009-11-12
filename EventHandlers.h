#include <Carbon/Carbon.h>

#define kInDirectoryParam 'inDr'

OSErr modulePathesHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr setAdditionalModulePathesHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);