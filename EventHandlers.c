#include "EventHandlers.h"

const AEEventHandlerInfo gEventInfo[] = {
{ kModuleLoaderSuite, kLoadModuleEvent, loadModuleHandler },
{ kModuleLoaderSuite, kFindModuleEvent, findModuleHandler}
// Add more suite/event/handler triplets here if you define more than one command.
};

const int kEventHandlerCount = (sizeof(gEventInfo) / sizeof(AEEventHandlerInfo));


void showAEDesc(const AppleEvent *ev)
{
	Handle result;
	OSStatus resultStatus;
	resultStatus = AEPrintDescToHandle(ev,&result);
	printf("%s\n",*result);
	DisposeHandle(result);
}

OSErr getStringValue(const AppleEvent *ev, AEKeyword theKey, CFStringRef *outStr)
{
#if useLog
	printf("start getStringValue\n");
#endif
	OSErr err;
	DescType typeCode;
	DescType returnedType;
    Size actualSize;
	Size dataSize;
	CFStringEncoding encodeKey;
	
	err = AESizeOfParam(ev, theKey, &typeCode, &dataSize);
	if (dataSize == 0) goto bail;
	
	switch (typeCode) {
		case typeChar:
			encodeKey = CFStringGetSystemEncoding();
			break;
		case typeUTF8Text:
			encodeKey = kCFStringEncodingUTF8;
			break;
		default :
			typeCode = typeUnicodeText;
			encodeKey = kCFStringEncodingUnicode;
	}
	
	UInt8 *dataPtr = malloc(dataSize);
	err = AEGetParamPtr (ev, theKey, typeCode, &returnedType, dataPtr, dataSize, &actualSize);
	if (actualSize > dataSize) {
#if useLog
		printf("buffere size is allocated. data:%i actual:%i\n", dataSize, actualSize);
#endif	
		dataSize = actualSize;
		dataPtr = (UInt8 *)realloc(dataPtr, dataSize);
		if (dataPtr == NULL) {
			printf("fail to reallocate memory\n");
			goto bail;
		}
		err = AEGetParamPtr (ev, theKey, typeCode, &returnedType, dataPtr, dataSize, &actualSize);
	}
	
	if (err != noErr) {
		free(dataPtr);
		goto bail;
	}
	
	*outStr = CFStringCreateWithBytes(NULL, dataPtr, dataSize, encodeKey, false);
	free(dataPtr);
bail:
#if useLog		
	printf("end getStringValue\n");
#endif
	return err;
}

int countEventHandlers()
{
	return sizeof(gEventInfo)/sizeof(AEEventHandlerInfo);
}

OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;
	
}

OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;
#if useLog	
	showAEDesc(ev);
#endif
	CFStringRef module_name = NULL;
	err = getStringValue(ev, keyDirectObject, &module_name);
	if (module_name == NULL) goto bail;
	
	CFStringRef additional_dir = NULL;
	err = getStringValue(ev, kInDirectoryParam, &additional_dir);
	/*
	 CFStringRef script_path = CFSTR("/Users/tkurita/Library/Scripts/Modules/DateObject.scpt");
	 CFURLRef script_url = CFURLCreateWithFileSystemPath(NULL, script_path, kCFURLPOSIXPathStyle, false);
	 FSRef ref;
	 CFURLGetFSRef(script_url, &ref);
	 */
	
	CFStringRef container_path = CFSTR("/Users/tkurita/Library/Scripts/Modules");
	CFURLRef container_url = CFURLCreateWithFileSystemPath(NULL, container_path, kCFURLPOSIXPathStyle, true);
	FSRef container_ref;
	CFURLGetFSRef(container_url, &container_ref);
	/*
	err = FSIterateContainer(&container_ref, 0, kFSCatInfoContentMod,
							 false, true, (IterateContainerFilterProcPtr)iterateFilter, &filterArg);
	*/
	
	goto bail;
	ComponentInstance scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
	OSAID script_id;
	OSAError osa_err;
	//osa_err = OSALoadFile(scriptingComponent, &(filterArg.targetRef), NULL, kOSAModeNull, &script_id);
	
	AEDesc result;
	osa_err = OSACoerceToDesc(scriptingComponent, script_id, typeWildCard,kOSAModeNull, &result);
	
	err = AEPutParamDesc(reply, keyAEResult, &result);
	
bail:	
	//--gAdditionReferenceCount;  // don't forget to decrement the reference count when you leave!
	//safeRelease(script_url);
	
	return err;
}
