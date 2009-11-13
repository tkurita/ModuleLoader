#include "EventHandlers.h"
#include "AEUtils.h"
#include "findModule.h"
#include "ModuleCache.h"

#define useModuleCache 0
#define useLog 0

static ComponentInstance scriptingComponent = NULL;
static OSAID LOADER_ID = kOSANullScript;

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
	
	return noErr;
}

OSErr setAdditionalModulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err;
	CFMutableArrayRef module_paths;	
	err = getPOSIXPathArray(ev, keyDirectObject, &module_paths);
	if (err != noErr) goto bail;
	setAdditionalModulePaths(module_paths);
bail:
	return err;
}

OSErr makeLocalLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;

	if (!scriptingComponent)
		scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
	
	return err;
}

OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err;
	if (LOADER_ID == kOSANullScript) {
		if (!scriptingComponent)
			scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
		CFBundleRef	bundle = CFBundleGetBundleWithIdentifier( CFSTR("Scriptfactory.ModuleLoaderOSAX") );
		CFURLRef loader_url = CFBundleCopyResourceURL(bundle, CFSTR("loader"), CFSTR("scpt"), CFSTR("Scripts"));
		FSRef loader_ref;
		CFURLGetFSRef(loader_url, &loader_ref);
		CFRelease(loader_url);
		err = OSALoadFile(scriptingComponent, &loader_ref, NULL, kOSAModeNull, &LOADER_ID);
		if (err != noErr) {
			putStringToEvent(reply, keyErrorString, 
							 CFSTR("Fail to load a script."), kCFStringEncodingUTF8);
			goto bail;
		}
	}
	AEDesc result;
	err = OSACoerceToDesc(scriptingComponent, LOADER_ID, typeWildCard,kOSAModeNull, &result);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
		goto bail;
	}
	err = AEPutParamDesc(reply, keyAEResult, &result);
bail:
	return err;
}

OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;
	CFStringRef module_name = NULL;
	err = getStringValue(ev, keyDirectObject, &module_name);
	if ((err != noErr) || (module_name == NULL)) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get a module name."), kCFStringEncodingUTF8);
		goto bail;
	}
	CFMutableArrayRef path_array = NULL;
	err = getPOSIXPathArray(ev, kInDirectoryParam, &path_array);
	
	Boolean with_other_paths = true;
	err = getBoolValue(ev, kOtherPathsParam, &with_other_paths);
	
	FSRef module_ref;
	err = findModule(module_name, (CFArrayRef)path_array, !with_other_paths, &module_ref);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("A module is not found."), kCFStringEncodingUTF8);
		goto bail;
	}
	AliasHandle an_alias;
	err = FSNewAlias(NULL, &module_ref, &an_alias);
	err = putAliasToReply(an_alias, reply);
bail:
	return err;
}

OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	OSErr err = noErr;
#if useLog	
	showAEDesc(ev);
#endif
#if useModuleCache	
	initializeCache();
#endif
	CFStringRef module_name = NULL;
	err = getStringValue(ev, keyDirectObject, &module_name);
	if ((err != noErr) || (module_name == NULL)) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get a module name."), kCFStringEncodingUTF8);
		goto bail;
	}

	CFMutableArrayRef path_array = NULL;
	err = getPOSIXPathArray(ev, kInDirectoryParam, &path_array);
	
	Boolean with_other_paths = true;
	err = getBoolValue(ev, kOtherPathsParam, &with_other_paths);
	
	OSAID script_id = kOSANullScript;
	OSAError osa_err = noErr;

	if (!scriptingComponent)
		scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);

#if useModuleCache	
	script_id = findModuleInCache(module_name);
	if (script_id == kOSANullScript) {
#endif
		CFStringRef additional_dir = NULL;
		err = getStringValue(ev, kInDirectoryParam, &additional_dir);
		FSRef module_ref;
		err = findModule(module_name, path_array, !with_other_paths, &module_ref);
		if (err != noErr) {
			putStringToEvent(reply, keyErrorString, 
							 CFSTR("A module is not found."), kCFStringEncodingUTF8);
			goto bail;
		}
		osa_err = OSALoadFile(scriptingComponent, &module_ref, NULL, kOSAModeNull, &script_id);
		if (osa_err != noErr) {
			err = osa_err;
			putStringToEvent(reply, keyErrorString, 
							 CFSTR("Fail to load a script."), kCFStringEncodingUTF8);
			goto bail;
		}
#if useModuleCache		
		storeModuleInCache(module_name, script_id);
	}
#endif	
	
	AEDesc result;
	osa_err = OSACoerceToDesc(scriptingComponent, script_id, typeWildCard,kOSAModeNull, &result);
	if (osa_err != noErr) {
		err = osa_err;
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
	} else {
		err = AEPutParamDesc(reply, keyAEResult, &result);
	}
	
	err = OSADispose(scriptingComponent,script_id);
	
bail:
	
	return err;
}
