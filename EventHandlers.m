#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include "EventHandlers.h"
#include "AEUtils.h"
#include "findModule.h"
#include "ModuleLoaderConstants.h"
#include "ExtractDependencies.h"
#import "AppleEventExtra.h"
#import <OSAKit/OSAKit.h>
#include <syslog.h>
#import "BAGenericObject.h"

#define useLog 0

extern const char *MODULE_SPEC_LABEL;

OSErr extractDependenciesASObjC(AEDesc *script_data_ptr, AEDesc *dependencies);

OSErr versionHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
#if useLog
	fprintf(stderr, "start versionHandler\n");
#endif
	OSErr err;	
	err = putStringToEvent(reply, keyAEResult, CFSTR(STR(ModuleLoader_VERSION)), kCFStringEncodingUnicode);
	return err;
}

OSErr modulePathsHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
	OSErr err;
    @autoreleasepool {
        NSMutableArray *all_paths = [NSMutableArray arrayWithCapacity:6];
        NSArray *additional_paths = additionalModulePaths();
        if (additional_paths) {
            [all_paths addObjectsFromArray:additional_paths];
        }
        
        NSArray *default_paths = copyDefaultModulePaths();
        if (default_paths) {
            [all_paths addObjectsFromArray:default_paths];
        }
        NSAppleEventDescriptor *list_desc = [all_paths appleEventDescriptor];
        err = AEPutParamDesc(reply, keyAEResult, [list_desc aeDesc]);
    }
	return err;
}

OSErr setAdditionalModulePathsHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
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

NSAppleEventDescriptor *loadBundleScript(NSString *name)
{
    NSBundle *bundle = [NSBundle bundleWithIdentifier:(__bridge NSString *)BUNDLE_ID];
    if (! bundle) return nil;
    NSURL *script_url = [bundle URLForResource:name withExtension:@"scpt"];
    if (! script_url) return nil;
    return [OSAScript scriptDataDescriptorWithContentsOfURL:script_url];
}

OSErr makeLocalLoaderHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
    OSErr err = noErr;
    @autoreleasepool {
        NSAppleEventDescriptor *script_desc = loadBundleScript(@"LocalLoader");
        if (! script_desc) {
            putStringToEvent(reply, keyErrorString,
                             CFSTR("Fail to load a local loader script."), kCFStringEncodingUTF8);
            err = kModuleLoaderInternalError;
        } else {
            err = AEPutParamDesc(reply, keyAEResult, [script_desc aeDesc]);
        }
    }
    return err;
}

OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
    OSErr err = noErr;
    @autoreleasepool {
        NSAppleEventDescriptor *script_desc = loadBundleScript(@"loader");
        if (! script_desc) {
            putStringToEvent(reply, keyErrorString,
                             CFSTR("Fail to load a loader script."), kCFStringEncodingUTF8);
            err = kModuleLoaderInternalError;
        } else {
            err = AEPutParamDesc(reply, keyAEResult, [script_desc aeDesc]);
        }
    }
    return err;
}

OSErr findModuleWithEvent(const AppleEvent *ev, AppleEvent *reply, ModuleRef** moduleRefPtr)
{
	OSErr err = noErr;
	NSMutableArray *searched_paths = nil;
	CFStringRef module_name = NULL;
	CFStringRef required_version = NULL;
	ModuleCondition *module_condition = NULL;
	CFStringRef errmsg =  NULL;
    AEDesc direct_object = {typeNull, NULL};
#if useLog
	showAEDesc(ev);
#endif
	Boolean with_other_paths = true;
	err = getBoolValue(ev, kOtherPathsParam, &with_other_paths);
	NSMutableArray *path_array = CFBridgingRelease(
                CFMutableArrayCreatePOSIXPathsWithEvent(ev, kInDirectoryParam, &err));
	
	err = AEGetParamDesc(ev, keyDirectObject, typeWildCard, &direct_object);
	if (noErr != err) goto bail;
	switch (direct_object.descriptorType) {
		case typeAERecord:
		case typeModuleSpecifier:
			module_name = CFStringCreateWithEvent(&direct_object, keyAEName, &err);
			ev = &direct_object;
			break;
		default:
			module_name = CFStringCreateWithEvent(ev, keyDirectObject, &err);
			break;
	}
	
	if ((err != noErr) || (module_name == NULL)) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get a module name."), kCFStringEncodingUTF8);
		goto bail;
	}
	
	required_version = CFStringCreateWithEvent(ev, kVersionParam, &err);	
    Boolean from_use = false;
    getBoolValue(ev, kFromUseParam,  &from_use);
    
	module_condition = ModuleConditionCreate(module_name, required_version, !from_use, &errmsg);
	if (!module_condition) {
		putStringToEvent(reply, keyErrorString, errmsg, kCFStringEncodingUTF8);
		err = kFailedToParseVersionCondition;
		goto bail;
	}

    err = findModule(module_condition, path_array,
                     !from_use, !with_other_paths,
                         moduleRefPtr, &searched_paths);
	if (err != noErr) {
        NSString *error_message;
		NSString *pathlist = nil;
		if (searched_paths) {
            pathlist = [searched_paths componentsJoinedByString:@":"];
		}
		
		if (required_version) {
            error_message = [NSString stringWithFormat:@"\"%@ (%@)\" can not be found in %@",
                      module_name,required_version, pathlist];
		} else {
            error_message = [NSString stringWithFormat:@"\"%@\" can not be found in %@",
                      module_name, pathlist];
		}
        AEPutParamDesc(reply, keyErrorString, [[error_message appleEventDescriptor] aeDesc]);
		goto bail;
	}
bail:
	safeRelease(module_name);
	safeRelease(required_version);
    safeRelease(errmsg);
	ModuleConditionFree(module_condition);
	AEDisposeDesc(&direct_object);
	return err;
}

OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
#if useLog
	fprintf(stderr, "start findModuleHandler\n");
#endif
	OSErr err = noErr;
	@autoreleasepool{
        ModuleRef* module_ref = NULL;
            err = findModuleWithEvent(ev, reply, &module_ref);
        
        if (err == noErr) {
            putFileURLToEvent(reply, keyAEResult, module_ref->url);
            ModuleRefFree(module_ref);
        }
    }
	return err;
}

OSErr setPropertyWithName(ComponentInstance scriptingComponent, OSAID scriptID, const char *propertyName, AEDesc *inDesc)
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

OSErr _loadModuleHandler_(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
	OSErr err = noErr;
    @autoreleasepool {
        OSAID script_id = kOSANullScript;
        ComponentInstance scriptingComponent = NULL;
        AEDescList dependencies = {typeNull, NULL};
        AEDesc furl_desc = {typeNull, NULL};
        AEDesc script_desc = {typeNull, NULL};
        AEDesc version_desc = {typeNull, NULL};
        AEDesc result_desc = {typeNull, NULL};
        
        ModuleRef* module_ref = NULL;
        err = findModuleWithEvent(ev, reply, &module_ref);
        if (err != noErr) goto bail;
        
        OSAError osa_err = noErr;
        
        
        scriptingComponent = [[[OSALanguage defaultLanguage] sharedLanguageInstance]
                                                componentInstance];
       
        
        osa_err = OSALoadFile(scriptingComponent, &(module_ref->fsref), NULL, kOSAModeCompileIntoContext, &script_id);
        if (osa_err != noErr) {
            err = osa_err;
            putStringToEvent(reply, keyErrorString, 
                             CFSTR("Fail to load a script."), kCFStringEncodingUTF8);
            goto bail;
        }
        
        err = AEDescCreateWithCFURL(module_ref->url, &furl_desc);
        if (noErr != err) goto bail;
        
        err = extractDependencies(scriptingComponent, script_id, &dependencies);
        if (noErr != err) goto bail;
        
        osa_err = OSACoerceToDesc(scriptingComponent, script_id, typeWildCard,kOSAModeNull, &script_desc);
        if (osa_err != noErr) {
            err = osa_err;
            putStringToEvent(reply, keyErrorString, CFSTR("Fail to OSACoerceToDesc."), kCFStringEncodingUTF8);
            goto bail;
        }
        
        if (module_ref->version) {
            AEDisposeDesc(&version_desc); // required to avoid memory leak. the reason is unknown.
            err = AEDescCreateWithCFString(module_ref->version, kCFStringEncodingUTF8, &version_desc);
            if (noErr != err) {
                putStringToEvent(reply, keyErrorString, CFSTR("Fail to AEDescCreateWithCFString."), kCFStringEncodingUTF8);
                goto bail;
            }
        } else {
            AEDisposeDesc(&version_desc); // required to avoid memory leak. the reason is unknown.
            err = AEDescCreateMissingValue(&version_desc);
            if (noErr != err) {
                putStringToEvent(reply, keyErrorString, CFSTR("Fail to AEDescCreateMissingValue."), kCFStringEncodingUTF8);
                goto bail;
            }		
        }
        
        AEBuildError ae_err;
        err = AEBuildDesc(&result_desc, &ae_err, "{file:@, scpt:@, DpIf:@, vers:@}",
                                        &furl_desc, &script_desc, &dependencies, &version_desc);
        if (noErr != err) goto bail;
        
        err = AEPutParamDesc(reply, keyAEResult, &result_desc);
         
    bail:
        AEDisposeDesc(&result_desc);
        AEDisposeDesc(&script_desc);
        AEDisposeDesc(&version_desc);
        AEDisposeDesc(&furl_desc);
        AEDisposeDesc(&dependencies);
        ModuleRefFree(module_ref);
        OSADispose(scriptingComponent, script_id);
    }
	return err;	
}

OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
#if useLog
	fprintf(stderr, "start loadModuleHandler\n");
#endif
    OSErr err = noErr;
	@autoreleasepool{
        ModuleRef *module_ref=NULL;
        err = findModuleWithEvent(ev, reply, &module_ref);
        if (noErr == err) {
            NSAppleEventDescriptor *script_desc = [OSAScript scriptDataDescriptorWithContentsOfURL:(__bridge NSURL * _Nonnull)(module_ref->url)];
            if (! script_desc) {
                putStringToEvent(reply, keyErrorString,
                             CFSTR("Fail to load a script."), kCFStringEncodingUTF8);
                err = kModuleLoaderInternalError;
                goto bail;
            }
            if ('scpt' != [script_desc descriptorType]) {
                NSError *osaerr = nil;
                OSALanguageInstance *langinst = [OSALanguageInstance languageInstanceWithLanguage:
                                                 [OSALanguage languageForName:@"AppleScript"]];
                OSAScript *scriptinst = [[OSAScript alloc] initWithScriptDataDescriptor:script_desc
                 fromURL:(__bridge NSURL * _Nonnull)(module_ref->url)
                                               languageInstance:langinst
                                            usingStorageOptions:OSANull
                                                          error:&osaerr];
                if (osaerr) {
                    NSLog(@"%@", osaerr);
                    putStringToEvent(reply, keyErrorString,
                                     CFSTR("Fail to initWithScriptDataDescriptor."), kCFStringEncodingUTF8);
                    err = kModuleLoaderInternalError;
                    goto bail;
                }
                NSDictionary *errdict = nil;
                NSData *scptdata = [scriptinst compiledDataForType:@"osas" usingStorageOptions:OSANull error:&errdict];
                if (errdict) {
                    NSLog(@"%@", errdict);
                    putStringToEvent(reply, keyErrorString,
                                     CFSTR("Fail to compiledDataForType."), kCFStringEncodingUTF8);
                    err = kModuleLoaderInternalError;
                    goto bail;
                }
                script_desc = [NSAppleEventDescriptor descriptorWithDescriptorType:typeScript data:scptdata];
            }
            err = AEPutParamDesc(reply, keyAEResult, [script_desc aeDesc]);
        }
bail:
        
        ModuleRefFree(module_ref);
    }
	return err;
}

OSErr makeModuleSpecHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
	OSErr err;
	AEDesc module_name = {typeNull, NULL};
	AEDesc module_spec = {typeNull, NULL};
	AEDesc required_version = {typeNull, NULL};
    AEDesc with_reloading = {typeFalse, NULL};
	AEBuildError ae_err;
	
	err = AEGetParamDesc(ev, keyDirectObject, typeWildCard, &module_name);
	if ((err == noErr) && (typeNull != module_name.descriptorType)) {
		err = AEBuildDesc(&module_spec, &ae_err, "MoSp{pnam:@}",&module_name);
		if (err != noErr) goto bail;
	} else {
		err = AEBuildDesc(&module_spec, &ae_err, "MoSp{}");
		if (err != noErr) goto bail;		
	}
	
	err = AEGetParamDesc(ev, kVersionParam, typeUnicodeText, &required_version);
	if ((err == noErr) && (typeNull != required_version.descriptorType)) {
		err = AEPutKeyDesc (&module_spec, kVersionParam, &required_version);
	}	
	
	err = AEGetParamDesc(ev, kReloadingParam, typeBoolean, &with_reloading);
	if ((err == noErr) && (typeNull != with_reloading.descriptorType)) {
		err = AEPutKeyDesc (&module_spec, kReloadingParam, &with_reloading);
	}
	
	err = AEPutParamDesc(reply, keyAEResult, &module_spec);

bail:
	AEDisposeDesc(&module_spec);
	AEDisposeDesc(&module_name);
	AEDisposeDesc(&required_version);
	AEDisposeDesc(&with_reloading);
	return err;
}

OSErr extractDependenciesHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
#if useLog
    syslog(LOG_NOTICE, "start extractDependenciesHandler");
    showAEDesc(ev);
#endif
	OSErr err;
	AEDesc script_data = {typeNull, NULL};
	AEDescList dependencies = {typeNull, NULL};
	err = AEGetParamDesc(ev, keyDirectObject, typeWildCard, &script_data);
    if (noErr != err ) {
        NSLog(@"%@", @"Faild to AEGetParamDesc in extractDependenciesHandler");
        return err;
    }
    AEKeyword desired_class;
    OSAID script_id = kOSANullScript;
    ComponentInstance scriptingComponent = NULL;
    @autoreleasepool {
        switch (script_data.descriptorType) {
            case typeScript:
                scriptingComponent = [[[OSALanguage defaultLanguage] sharedLanguageInstance]
                                      componentInstance];
                
                err = OSACoerceFromDesc(scriptingComponent, &script_data, kOSAModeNull, &script_id);
                if (err != noErr) goto bail;
                break;
            case typeObjectSpecifier:
                err = AEGetKeyPtr(&script_data, keyAEDesiredClass, typeType, NULL, &desired_class, sizeof(desired_class), NULL);
                if (desired_class != 'ocid') {
                    err = 1804;
                    break;
                }
                err = extractDependenciesASObjC(&script_data, &dependencies);
                if (noErr != err) goto bail;
                goto putparam;
                break;
            default:
                err = 1804;
        }
        if (err != noErr) {
            NSLog(@"%@", @"Faild to AEGetParamDesc in extractDependenciesHandler");
            showAEDesc(ev);
            goto bail;
        }

        err = extractDependencies(scriptingComponent, script_id, &dependencies);
        if (noErr != err) goto bail;
putparam:
        err = AEPutParamDesc(reply, keyAEResult, &dependencies);
bail:
        OSADispose(scriptingComponent, script_id);
        AEDisposeDesc(&script_data);
        AEDisposeDesc(&dependencies);
    }
	return err;
}

OSErr meetVersionHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon)
{
	OSErr err = noErr;
	CFStringRef version = NULL;
	CFStringRef condition = NULL;
	VersionConditionSet *vercond_set = NULL;
	CFStringRef errmsg = NULL;
	
	version = CFStringCreateWithEvent(ev, keyDirectObject, &err);
	if (noErr != err) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get version number."), kCFStringEncodingUTF8);
		goto bail;
	}
	condition = CFStringCreateWithEvent(ev, kConditionParam, &err);
	if (noErr != err) {
		putStringToEvent(reply, keyErrorString, 
						 CFSTR("Failed to get condition parameter."), kCFStringEncodingUTF8);
		goto bail;
	}
	
	vercond_set = VersionConditionSetCreate(condition, &errmsg);
	if (errmsg) {
		putStringToEvent(reply, keyErrorString, errmsg, kCFStringEncodingUTF8);
		err = kFailedToParseVersionCondition;
		goto bail;
	}
	Boolean result = VersionConditionSetIsSatisfied(vercond_set, version);
	err = putBooleanToEvent(reply, keyAEResult, result);
	
bail:
	safeRelease(version);
	safeRelease(condition);
	VersionConditionSetFree(vercond_set);
	return err;
}
