#include <Carbon/Carbon.h>

#define kInDirectoryParam 'inDr'
#define kOtherPathsParam 'ohPh'

OSErr makeLocalLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr modulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr setAdditionalModulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);