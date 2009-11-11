#include "EventHandlers.h"
#include "AEUtils.h"
#include "findModule.h"
#include "ModuleCache.h"


static ComponentInstance scriptingComponent = NULL;
static OSAID LOADER_ID = kOSANullScript;

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
	FSRef module_ref;
	err = findModule(module_name, &module_ref);
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
	initializeCache();
	CFStringRef module_name = NULL;
	err = getStringValue(ev, keyDirectObject, &module_name);
	if ((err != noErr) || (module_name == NULL)) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get a module name."), kCFStringEncodingUTF8);
		goto bail;
	}

	OSAID script_id;
	OSAError osa_err;

	 //ComponentInstance scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
	if (!scriptingComponent)
		scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
	
	script_id = findModuleInCache(module_name);
	if (script_id == kOSANullScript) {
		CFStringRef additional_dir = NULL;
		err = getStringValue(ev, kInDirectoryParam, &additional_dir);
		FSRef module_ref;
		err = findModule(module_name, &module_ref);
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
		storeModuleInCache(module_name, script_id);
	}
	
	
	AEDesc result;
	osa_err = OSACoerceToDesc(scriptingComponent, script_id, typeWildCard,kOSAModeNull, &result);
	if (osa_err != noErr) {
		err = osa_err;
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
		goto bail;
	}
	err = AEPutParamDesc(reply, keyAEResult, &result);
	
bail:
	
	return err;
}
