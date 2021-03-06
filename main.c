#include "findModule.h"
#include "EventHandlers.h"
#include "ModuleLoaderConstants.h"

#define useLog 0

CFBundleRef		gAdditionBundle;

struct AEEventHandlerInfo {
	FourCharCode			evClass, evID;
	AEEventHandlerProcPtr	proc;
};

typedef struct AEEventHandlerInfo AEEventHandlerInfo;

static AEEventHandlerUPP *gEventUPPs;

// =============================================================================
// == Entry points.

static OSErr InstallMyEventHandlers();
static void RemoveMyEventHandlers();

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
	RemoveMyEventHandlers();
	free(gEventUPPs);
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

#if !__LP64__
int main(int argc, char *argv[]) // for debugging
{
	/*
	CFURLRef url = CFURLCreateWithFileSystemPath(NULL,
										   CFSTR("/Users/tkurita/Downloads/CFPreferences/"),
										   kCFURLPOSIXPathStyle, true);
	CFMutableArrayRef array = CFArrayCreateMutable(NULL, 1, &kCFTypeArrayCallBacks);
	//CFArrayAppendValue(array, url);
	CFArrayAppendValue(array, CFSTR("/Users/tkurita/Downloads/CFPreferences/"));
	setAdditionalModulePaths(array);
	 */
	/*
	CFStringRef errmsg = NULL;
	CFMutableArrayRef searchedPaths = NULL;
	FSRef moduleRef;
	//ModuleCondition *module_condition = ModuleConditionCreate(CFSTR("Module$V"), CFSTR("<= 1.2"), &errmsg);
	ModuleCondition *module_condition = ModuleConditionCreate(CFSTR("SampleModules:ModuleA"), NULL, &errmsg);
	OSErr err = findModule(module_condition, NULL, false, &moduleRef, &searchedPaths);
	if (noErr == err) fprintf(stderr, "No error\n");
	*/
	gEventUPPs = malloc(sizeof(AEEventHandlerUPP)*kEventHandlerCount);
	InstallMyEventHandlers();
    RunApplicationEventLoop();
	RemoveMyEventHandlers();
	free(gEventUPPs);
	return 0;
}
#endif