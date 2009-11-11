#include <Carbon/Carbon.h>

struct AEEventHandlerInfo {
	FourCharCode			evClass, evID;
	AEEventHandlerProcPtr	proc;
};

typedef struct AEEventHandlerInfo AEEventHandlerInfo;

#define kModuleLoaderSuite  'Molo'
#define kLoadModuleEvent	'loMo'
#define kInDirectoryParam 'inDr'
#define kFindModuleEvent 'fdMo'

OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon);
int countEventHandlers();