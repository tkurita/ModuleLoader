#include "EventHandlers.h"
#include "AEUtils.h"
#include "findModule.h"
#include "ModuleLoaderConstants.h"

#define useLog 0
#define CACHE_LOADER_SCRIPT 1


static ComponentInstance scriptingComponent = NULL;
static OSAID LOADER_ID = kOSANullScript;
static OSAID LOCAL_LOADER_ID = kOSANullScript;

OSErr versionHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err;
	CFBundleRef	bundle = CFBundleGetBundleWithIdentifier(BUNDLE_ID);
	CFDictionaryRef info = CFBundleGetInfoDictionary(bundle);
	
	CFStringRef vers = CFDictionaryGetValue(info, CFSTR("CFBundleShortVersionString"));
	err = putStringToEvent(reply, keyAEResult, vers, kCFStringEncodingUnicode);
	//err = putStringToEvent(reply, keyAEResult, vers, kCFStringEncodingUTF8);
	return err;
}

OSErr modulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err;
	
	CFMutableArrayRef all_paths = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
	CFArrayRef additional_paths = additionalModulePaths();
	if (additional_paths) {
		CFArrayAppendArray(all_paths, additional_paths, 
						   CFRangeMake(0, CFArrayGetCount(additional_paths)));
	}
	
	CFArrayRef default_paths = copyDefaultModulePaths();
	if (default_paths) {
		CFArrayAppendArray(all_paths, default_paths, 
						   CFRangeMake(0, CFArrayGetCount(default_paths)));
		CFRelease(default_paths);
	}
	
	err = putStringListToEvent(reply, keyAEResult, all_paths, kCFStringEncodingUTF8);
	CFRelease(all_paths);
	return noErr;
}

OSErr setAdditionalModulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err;
	CFMutableArrayRef module_paths = NULL;
	Boolean ismsg;
	err = isMissingValue(ev, keyDirectObject, &ismsg);
	if (!ismsg) {
		module_paths = CFMutableArrayCreatePOSIXPathsWithEvent(ev, keyDirectObject, &err);
	}
	if (err != noErr) goto bail;
	if (module_paths && CFArrayGetCount(module_paths)) {
		setAdditionalModulePaths(module_paths);
	} else {
		setAdditionalModulePaths(NULL);
	}
bail:
	safeRelease(module_paths);
	return err;
}


OSErr loadBundleScript(CFStringRef name, OSAID *newIDPtr)
{
	OSErr err = noErr;
	if (*newIDPtr != kOSANullScript) goto bail;
	
	if (!scriptingComponent)
		scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
	CFBundleRef	bundle = CFBundleGetBundleWithIdentifier(BUNDLE_ID);
	CFURLRef script_url = CFBundleCopyResourceURL(bundle, name, CFSTR("scpt"), NULL);
	if (!script_url) {
		fprintf(stderr, "Fail to load a script.\n");
		err = 1802;
		goto bail;
	}
	FSRef script_ref;
	CFURLGetFSRef(script_url, &script_ref);
	CFRelease(script_url);
	err = OSALoadFile(scriptingComponent, &script_ref, NULL, kOSAModeNull, newIDPtr);
	if (err != noErr) {
		goto bail;
	}

bail:
	return err;
}

OSErr makeLocalLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;
	
	err = loadBundleScript(CFSTR("LocalLoader"), &LOCAL_LOADER_ID);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to load a script."), kCFStringEncodingUTF8);
		goto bail;
	}
	
	AppleEvent setup_event;
    ProcessSerialNumber PSN = {0, kCurrentProcess}; 
	AEBuildError aeerr;
	err = AEBuildAppleEvent(kASAppleScriptSuite, kASSubroutineEvent, 
							typeProcessSerialNumber, (Ptr) &PSN, sizeof(PSN), 
							kAutoGenerateReturnID, kAnyTransactionID, 
							&setup_event, 
							&aeerr, 
							"'snam':TEXT(@)", 
							"setup"); 
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to AEBuildAppleEvent."), kCFStringEncodingUTF8);	
		goto bail;
	}
	
	OSAID resultID = kOSANullScript;
	err = OSAExecuteEvent(scriptingComponent, &setup_event,
						  LOCAL_LOADER_ID, kOSAModeNull, &resultID);
	AEDisposeDesc(&setup_event);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to setup a local loader."), kCFStringEncodingUTF8);
		goto bail;		
	}
	
	AEDesc result;
	err = OSACoerceToDesc(scriptingComponent, LOCAL_LOADER_ID, typeWildCard,kOSAModeNull, &result);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
		goto bail;
	}
	err = AEPutParamDesc(reply, keyAEResult, &result);
	AEDisposeDesc(&result);
bail:
	if ((!CACHE_LOADER_SCRIPT) && (LOCAL_LOADER_ID != kOSANullScript)) {
		OSADispose(scriptingComponent, LOCAL_LOADER_ID);
		LOCAL_LOADER_ID = kOSANullScript;
	}
	return err;
}

OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err;
	err = loadBundleScript(CFSTR("loader"), &LOADER_ID);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to load a script."), kCFStringEncodingUTF8);
		goto bail;
	}
	
	AEDesc result;
	err = OSACoerceToDesc(scriptingComponent, LOADER_ID, typeWildCard,kOSAModeNull, &result);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
		goto bail;
	}
	err = AEPutParamDesc(reply, keyAEResult, &result);
	AEDisposeDesc(&result);
bail:
	if ((!CACHE_LOADER_SCRIPT) && (LOADER_ID != kOSANullScript)) {
		OSADispose(scriptingComponent, LOADER_ID);
		LOADER_ID = kOSANullScript;
	}
	return err;
}

OSErr findModuleWithEvent(const AppleEvent *ev, AppleEvent *reply, FSRef* moduleRefPtr)
{
	OSErr err = noErr;
	CFMutableArrayRef searched_paths = NULL;
	CFMutableArrayRef path_array = NULL;
	
	CFStringRef module_name = CFStringCreateWithEvent(ev, keyDirectObject, &err);
	if ((err != noErr) || (module_name == NULL)) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get a module name."), kCFStringEncodingUTF8);
		goto bail;
	}
	
	searched_paths = CFMutableArrayCreatePOSIXPathsWithEvent(ev, kInDirectoryParam, &err);
	
	Boolean with_other_paths = true;
	err = getBoolValue(ev, kOtherPathsParam, &with_other_paths);
	
	err = findModule(module_name, (CFArrayRef)path_array, !with_other_paths, 
					 moduleRefPtr, &searched_paths);
	if (err != noErr) {
		CFStringRef pathlist = CFStringCreateByCombiningStrings(NULL, searched_paths, CFSTR(":"));
		
		CFStringRef msg = CFStringCreateWithFormat(NULL, NULL, CFSTR("\"%@\" can not be found in %@"),
												   module_name, pathlist);
		putStringToEvent(reply, keyErrorString, msg, kCFStringEncodingUTF8);
		CFRelease(pathlist);
		CFRelease(msg);
		goto bail;
	}
bail:
	safeRelease(module_name);
	safeRelease(path_array);
	safeRelease(searched_paths);
	return err;
}

OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;
	FSRef module_ref;
	err = findModuleWithEvent(ev, reply, &module_ref);
	if (err != noErr) goto bail;
	AliasHandle an_alias;
	err = FSNewAlias(NULL, &module_ref, &an_alias);
	err = putAliasToReply(an_alias, reply);
bail:
	return err;
}

OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;
	FSRef module_ref;
	err = findModuleWithEvent(ev, reply, &module_ref);
	if (err != noErr) goto bail;
	
	OSAID script_id = kOSANullScript;
	OSAError osa_err = noErr;
	if (!scriptingComponent)
		scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);

	osa_err = OSALoadFile(scriptingComponent, &module_ref, NULL, kOSAModeNull, &script_id);
	if (osa_err != noErr) {
		err = osa_err;
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to load a script."), kCFStringEncodingUTF8);
		goto bail;
	}

	AEDesc result;
	osa_err = OSACoerceToDesc(scriptingComponent, script_id, typeWildCard,kOSAModeNull, &result);
	if (osa_err != noErr) {
		err = osa_err;
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
	} else {
		err = AEPutParamDesc(reply, keyAEResult, &result);
	}
	AEDisposeDesc(&result);
	OSADispose(scriptingComponent,script_id);
	
bail:
	
	return err;
}
