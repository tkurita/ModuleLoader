#include "EventHandlers.h"
#include "AEUtils.h"
#include "findModule.h"

const AEEventHandlerInfo gEventInfo[] = {
{ kModuleLoaderSuite, kLoadModuleEvent, loadModuleHandler },
{ kModuleLoaderSuite, kFindModuleEvent, findModuleHandler}
// Add more suite/event/handler triplets here if you define more than one command.
};

const int kEventHandlerCount = (sizeof(gEventInfo) / sizeof(AEEventHandlerInfo));

int countEventHandlers()
{
	return sizeof(gEventInfo)/sizeof(AEEventHandlerInfo);
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
	CFStringRef module_name = NULL;
	err = getStringValue(ev, keyDirectObject, &module_name);
	if ((err != noErr) || (module_name == NULL)) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get a module name."), kCFStringEncodingUTF8);
		goto bail;
	}
	
	CFStringRef additional_dir = NULL;
	err = getStringValue(ev, kInDirectoryParam, &additional_dir);
	FSRef module_ref;
	err = findModule(module_name, &module_ref);
	if (err != noErr) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("A module is not found."), kCFStringEncodingUTF8);
		goto bail;
	}
	ComponentInstance scriptingComponent = OpenDefaultComponent(kOSAComponentType, kAppleScriptSubtype);
	OSAID script_id;
	OSAError osa_err;
	osa_err = OSALoadFile(scriptingComponent, &module_ref, NULL, kOSAModeNull, &script_id);
	
	AEDesc result;
	osa_err = OSACoerceToDesc(scriptingComponent, script_id, typeWildCard,kOSAModeNull, &result);
	
	err = AEPutParamDesc(reply, keyAEResult, &result);
	
bail:
	
	return err;
}
