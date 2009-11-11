#include "findModule.h"
#include "EventHandlers.h"

#define useLog 1

extern const int kEventHandlerCount;
extern AEEventHandlerInfo gEventInfo[];

UInt32			gAdditionReferenceCount = 0;
CFBundleRef		gAdditionBundle;

static AEEventHandlerUPP *gEventUPPs;

// =============================================================================
// == Entry points.

static OSErr InstallMyEventHandlers();
static void RemoveMyEventHandlers();

OSErr SAInitialize(CFBundleRef theBundle)
{
#if useLog
	printf("start SAInitialize\n");
#endif	
	gAdditionBundle = theBundle;  // no retain needed.
	gEventUPPs = malloc(sizeof(AEEventHandlerUPP)*kEventHandlerCount);
	return InstallMyEventHandlers();
}

void SATerminate()
{
	free(gEventUPPs);
	RemoveMyEventHandlers();
}

Boolean SAIsBusy()
{
	//return gAdditionReferenceCount != 0;
	return true;
}

static OSErr InstallMyEventHandlers()
{
#if useLog
	printf("start InstallMyEventHandlers\n");
#endif
	OSErr		err;
	for (size_t i = 0; i < kEventHandlerCount; ++i) {
		if ((gEventUPPs[i] = NewAEEventHandlerUPP(gEventInfo[i].proc)) != NULL) {
			#if useLog
				printf("before AEInstallEventHandler\n");
			#endif
			err = AEInstallEventHandler(gEventInfo[i].evClass, gEventInfo[i].evID, gEventUPPs[i], 0, true);
		}
		else {
			err = memFullErr;
		}
		
		if (err != noErr) {
			SATerminate();  // Call the termination function ourselves, because the loader won't once we fail.
			return err;
		}
	}
	
	return noErr; 
}

static void RemoveMyEventHandlers()
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

int main(int argc, char *argv[]) // for debugging
{
	gEventUPPs = malloc(sizeof(AEEventHandlerUPP)*kEventHandlerCount);
	InstallMyEventHandlers();
    RunApplicationEventLoop();
	RemoveMyEventHandlers();
	free(gEventUPPs);
	return 0;
}
