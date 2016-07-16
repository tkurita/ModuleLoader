#import <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>
#import "InstallAE.h"
#include "findModule.h"
#include "EventHandlers.h"
#include "ModuleLoaderConstants.h"


#define useLog 0

struct AEEventHandlerInfo {
	FourCharCode			evClass, evID;
	AEEventHandlerProcPtr	proc;
};

typedef struct AEEventHandlerInfo AEEventHandlerInfo;

static AEEventHandlerUPP *gEventUPPs;

// =============================================================================
// == Entry points.


const AEEventHandlerInfo gEventInfo[] = {
{ kModuleLoaderSuite, kLoadModuleEvent, loadModuleHandler },
{ kModuleLoaderSuite, kPrivateLoadModuleEvent, _loadModuleHandler_ },
{ kModuleLoaderSuite, kFindModuleEvent, findModuleHandler},
{ kModuleLoaderSuite, kMakeLoaderEvent, makeLoaderHandler},
{ kModuleLoaderSuite, kPrivateMakeLoaderEvent, makeLoaderHandler},
{ kModuleLoaderSuite, kSetAdditionalModulePathsEvent, setAdditionalModulePathsHandler},
{ kModuleLoaderSuite, kModulePathsEvent, modulePathsHandler},
{ kModuleLoaderSuite, kMakeLocaLoaderEvent, makeLocalLoaderHandler},
{ kModuleLoaderSuite, kVersionEvent, versionHandler},
{ kModuleLoaderSuite, kMakeModuleSpecEvent, makeModuleSpecHandler},
{ kModuleLoaderSuite, kExtractDependenciesEvent, extractDependenciesHandler},
{ kModuleLoaderSuite, kMeetVersionEvent, meetVersionHandler}
};

const int kEventHandlerCount = (sizeof(gEventInfo) / sizeof(AEEventHandlerInfo));

int countEventHandlers()
{
	return sizeof(gEventInfo)/sizeof(AEEventHandlerInfo);
}

void SATerminate()
{
	RemoveMyEventHandlers();
	free(gEventUPPs);
}

OSErr InstallMyEventHandlers()
{
#if useLog
	printf("start InstallMyEventHandlers\n");
#endif
	OSErr		err;
    gEventUPPs = malloc(sizeof(AEEventHandlerUPP)*kEventHandlerCount);
	for (size_t i = 0; i < kEventHandlerCount; ++i) {
		if ((gEventUPPs[i] = NewAEEventHandlerUPP(gEventInfo[i].proc)) != NULL) {
			#if useLog
				fprintf(stderr, "before AEInstallEventHandler %d\n", i);
			#endif
			err = AEInstallEventHandler(gEventInfo[i].evClass, gEventInfo[i].evID, gEventUPPs[i], 0, true);
		}
		else {
			err = memFullErr;
		}
		
		if (err != noErr) {
			fprintf(stderr, "Failed to Install Event handler.\n");
			SATerminate();  // Call the termination function ourselves, because the loader won't once we fail.
			return err;
		}
	}
	
	return noErr; 
}

void RemoveMyEventHandlers()
{
	OSErr		err;
	size_t		i;
#if useLog	
	printf("start RemoveMyEventHandlers\n");
#endif	
	for (i = 0; i < kEventHandlerCount; ++i) {
		//printf("%i\n",i);
		if (gEventUPPs[i] &&
			(err = AERemoveEventHandler(gEventInfo[i].evClass, gEventInfo[i].evID, gEventUPPs[i], true)) == noErr)
		{
			DisposeAEEventHandlerUPP(gEventUPPs[i]);
			gEventUPPs[i] = NULL;
		}
	}
#if useLog
	printf("end RemoveMyEventHandlers\n");
#endif	
}
