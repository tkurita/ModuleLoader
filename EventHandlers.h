#include <Carbon/Carbon.h>

OSErr versionHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr makeLocalLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr modulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr setAdditionalModulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr makeModuleSpecHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
