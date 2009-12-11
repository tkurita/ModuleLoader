#include "EventHandlers.h"
#include "AEUtils.h"
#include "findModule.h"
#include "ModuleLoaderConstants.h"
#include "ExtractDependencies.h"

#define useLog 0

UInt32 gAdditionReferenceCount = 0;

static ComponentInstance scriptingComponent = NULL;

extern const char *MODULE_SPEC_LABEL;
extern const char *MODULE_DEPENDENCIES_LABEL;
static const char *MODULE_PATH_LABEL = "__module_path__";

OSErr versionHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
	OSErr err;
	CFBundleRef	bundle = CFBundleGetBundleWithIdentifier(BUNDLE_ID);
	CFDictionaryRef info = CFBundleGetInfoDictionary(bundle);
	
	CFStringRef vers = CFDictionaryGetValue(info, CFSTR("CFBundleShortVersionString"));
	err = putStringToEvent(reply, keyAEResult, vers, kCFStringEncodingUnicode);
	gAdditionReferenceCount--;
	return err;
}

OSErr modulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
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
	gAdditionReferenceCount--;
	return noErr;
}

OSErr setAdditionalModulePathsHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
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
	gAdditionReferenceCount--;	
	return err;
}


OSErr loadBundleScript(CFStringRef name, OSAID *newIDPtr)
{
	gAdditionReferenceCount++;
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
	gAdditionReferenceCount--;
	return err;
}

OSErr makeLocalLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
	OSErr err = noErr;
	OSAID loader_id = kOSANullScript;
	OSAID result_id = kOSANullScript;
	
	err = loadBundleScript(CFSTR("LocalLoader"), &loader_id);
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
	
	err = OSAExecuteEvent(scriptingComponent, &setup_event,
						  loader_id, kOSAModeNull, &result_id);
	AEDisposeDesc(&setup_event);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to setup a local loader."), kCFStringEncodingUTF8);
		goto bail;		
	}
	
	AEDesc result;
	err = OSACoerceToDesc(scriptingComponent, loader_id, typeWildCard,kOSAModeNull, &result);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
		goto bail;
	}
	err = AEPutParamDesc(reply, keyAEResult, &result);
	AEDisposeDesc(&result);
bail:
	OSADispose(scriptingComponent, loader_id);
	OSADispose(scriptingComponent, result_id);
	gAdditionReferenceCount--;
	return err;
}

OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
	OSErr err;
	OSAID loader_id = kOSANullScript;
	err = loadBundleScript(CFSTR("loader"), &loader_id);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to a loader script."), kCFStringEncodingUTF8);
		goto bail;
	}
	
	AEDesc result;
	err = OSACoerceToDesc(scriptingComponent, loader_id, typeWildCard, kOSAModeNull, &result);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
		goto bail;
	}
	err = AEPutParamDesc(reply, keyAEResult, &result);
	AEDisposeDesc(&result);
bail:
	OSADispose(scriptingComponent, loader_id);
	gAdditionReferenceCount--;
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
	
	path_array = CFMutableArrayCreatePOSIXPathsWithEvent(ev, kInDirectoryParam, &err);
	
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
	gAdditionReferenceCount++;
	OSErr err = noErr;
	FSRef module_ref;
	err = findModuleWithEvent(ev, reply, &module_ref);
	if (err != noErr) goto bail;
	AliasHandle an_alias;
	err = FSNewAlias(NULL, &module_ref, &an_alias);
	err = putAliasToReply(an_alias, reply);
	DisposeHandle((Handle) an_alias);
bail:
	gAdditionReferenceCount--;
	return err;
}

OSErr setPropertyWithName(OSAID scriptID, const char *propertyName, AEDesc *inDesc)
{
	OSErr err = noErr;
	AEDesc a_label;
	AECreateDesc(typeChar, propertyName, strlen(propertyName), &a_label);
	OSAID value_id = kOSANullScript;
	err = OSACoerceFromDesc(scriptingComponent, inDesc, kOSAModeNull, &value_id);
	if (noErr != err) goto bail;
	err = OSASetProperty(scriptingComponent, kOSAModeNull, scriptID, &a_label, value_id);
	if (noErr != err) goto bail;
bail:
	OSADispose(scriptingComponent, value_id);
	AEDisposeDesc(&a_label);
	return err;
}

OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
	OSErr err = noErr;
	OSAID script_id = kOSANullScript;
	
	AEDescList dependencies;
	AECreateDesc(typeNull, NULL, 0, &dependencies);
	AEDesc module_spec;
	AECreateDesc(typeNull, NULL, 0, &module_spec);
	AEDesc alias_desc;
	AECreateDesc(typeNull, NULL, 0, &alias_desc);
	
	FSRef module_ref;
	err = findModuleWithEvent(ev, reply, &module_ref);
	if (err != noErr) goto bail;
	
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
	Boolean append_info = false;
	err = getBoolValue(ev, kAppendInfoParam, &append_info);
	if (append_info) {
		// add __module_path__ property
		AliasHandle an_alias;
		err = FSNewAlias(NULL, &module_ref, &an_alias);
		HLock((Handle)an_alias);
		err = AECreateDesc(typeAlias, (Ptr) (*an_alias),
						   GetHandleSize((Handle) an_alias), &alias_desc);
		HUnlock((Handle)an_alias);
		if (noErr != err) goto bail;
		err = setPropertyWithName(script_id, MODULE_PATH_LABEL, &alias_desc);
		DisposeHandle((Handle) an_alias);
		if (noErr != err) goto bail;
		
		// add __module_dependencies__ property
		err = extractDependencies(scriptingComponent, script_id, &dependencies);
		if (noErr != err) goto bail;
		err = setPropertyWithName(script_id, MODULE_DEPENDENCIES_LABEL, &dependencies);
		if (noErr != err) goto bail;
		
		// add __module_spec_ property
		AEDesc module_name;
		err = AEGetParamDesc(ev, keyDirectObject, typeWildCard, &module_name);
		if (err != noErr) goto bail;
		AEBuildError ae_err;
		err = AEBuildDesc(&module_spec, &ae_err, "MoSp{pnam:@}",&module_name);
		AEDisposeDesc(&module_name);
		if (err != noErr) goto bail;
		err = setPropertyWithName(script_id, MODULE_SPEC_LABEL, &module_spec);
		if (noErr != err) goto bail;
	}
	
	// setup result
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
	
bail:
	AEDisposeDesc(&alias_desc);
	AEDisposeDesc(&module_spec);
	AEDisposeDesc(&dependencies);
	OSADispose(scriptingComponent, script_id);
	gAdditionReferenceCount--;
	return err;
}

OSErr makeModuleSpecHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
	OSErr err;
	AEDesc module_name;
	AECreateDesc(typeNull, NULL, 0, &module_name);
	AEDesc module_spec;
	AECreateDesc(typeNull, NULL, 0, &module_spec);

	AEBuildError ae_err;
	
	err = AEGetParamDesc(ev, keyDirectObject, typeWildCard, &module_name);
	if ((err == noErr) && (typeNull != module_name.descriptorType)) {
		err = AEBuildDesc(&module_spec, &ae_err, "MoSp{pnam:@}",&module_name);
		if (err != noErr) goto bail;
	} else {
		err = AEBuildDesc(&module_spec, &ae_err, "MoSp{}");
		if (err != noErr) goto bail;		
	}
	
	err = AEPutParamDesc(reply, keyAEResult, &module_spec);

bail:
	AEDisposeDesc(&module_spec);
	AEDisposeDesc(&module_name);
	gAdditionReferenceCount--;
	return err;
}

OSErr extractDependenciesHandler(const AppleEvent *ev, AppleEvent *reply, long refcon)
{
	gAdditionReferenceCount++;
	OSErr err;
	AEDesc script_data;
	AECreateDesc(typeNull, NULL, 0, &script_data);
	AEDescList dependencies;
	AECreateDesc(typeNull, NULL, 0, &dependencies);
	err = AEGetParamDesc(ev, keyDirectObject, typeScript, &script_data);
	if (err != noErr) goto bail;
	if (!scriptingComponent)
		scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
	
	OSAID script_id = kOSANullScript;
	err = OSACoerceFromDesc(scriptingComponent, &script_data, kOSAModeNull, &script_id);
	if (err != noErr) goto bail;

	err = extractDependencies(scriptingComponent, script_id, &dependencies);
	if (noErr != err) goto bail;
	err = AEPutParamDesc(reply, keyAEResult, &dependencies);
bail:
	AEDisposeDesc(&script_data);
	AEDisposeDesc(&dependencies);
	gAdditionReferenceCount--;
	return err;
}