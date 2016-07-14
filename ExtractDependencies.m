#import <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>
#include "ModuleLoaderConstants.h"
#include "ExtractDependencies.h"
#include "AEUtils.h"


#define useLog 0

const char *MODULE_SPEC_LABEL = "__module_specifier__";
const char *MODULE_DEPENDENCIES_LABEL = "__module_dependencies__";


OSErr ConvertToModuleSpecifier(AEDesc *ae_desc, AEDesc *modspec,
                               NSAppleEventDescriptor *reqested_items, Boolean *ismodule)
{
    OSErr err;
    AEKeyword desired_class;
    AEDesc a_pname;
    AECreateDesc(typeNull, NULL, 0, &a_pname);
    NSAppleEventDescriptor *vers = nil;
    NSString *libname = nil;
    *ismodule = false;
    
    err = AEGetKeyPtr(ae_desc, keyAEDesiredClass, typeType, NULL, &desired_class, sizeof(desired_class), NULL);
    if (noErr != err) goto bail;
    if (desired_class != typeScript) goto bail;
    err = AEGetKeyDesc(ae_desc, keyAEKeyData, typeUnicodeText, &a_pname);
    if (noErr != err) goto bail;
    libname = [[[NSAppleEventDescriptor alloc] initWithAEDescNoCopy:&a_pname] stringValue];
    for (NSInteger n = 1; n <= [reqested_items numberOfItems]; n++) {
        NSAppleEventDescriptor *a_record = [reqested_items descriptorAtIndex:n];
        if ([libname isEqualToString:[[[a_record descriptorForKeyword:cObject] descriptorForKeyword:keyAEKeyData] stringValue]]) {
            vers = [a_record descriptorForKeyword: keyAEVersion];
            break;
        }
    }

    AEBuildError ae_err;
    if (vers) {
        err = AEBuildDesc(modspec, &ae_err, "MoSp{pnam:@, vers:@}",&a_pname, [vers aeDesc]);
    } else {
        err = AEBuildDesc(modspec, &ae_err, "MoSp{pnam:@}",&a_pname);
    }
    if (noErr != err) goto bail;
    *ismodule = true;
bail:
    return err;
}

OSErr extractDependencies(ComponentInstance component, OSAID scriptID, AEDescList *dependencies)
{
	OSErr err;
	
	AEDescList property_names;
	AECreateDesc(typeNull, NULL, 0, &property_names);	
    AEDesc true_desc;
    AECreateDesc(typeTrue, NULL, 0, &true_desc);
    OSAID reqitems_id = kOSANullScript;
    AEDescList reqitems_desc;
    AECreateDesc(typeTrue, NULL, 0, &reqitems_desc);
    NSAppleEventDescriptor *reqested_items = nil;
	AEDesc moduleSpecLabel;
	err = AECreateDesc(typeChar, MODULE_SPEC_LABEL, strlen(MODULE_SPEC_LABEL), &moduleSpecLabel);
	AEDesc moduleDependenciesLabel;
	err = AECreateDesc(typeChar, MODULE_DEPENDENCIES_LABEL, strlen(MODULE_DEPENDENCIES_LABEL), &moduleDependenciesLabel);

	err = AECreateList(NULL, 0, false, dependencies);
	if (err != noErr) goto bail;
	
	OSAID deplist_id = kOSANullScript;
	err = OSAGetProperty(component, kOSAModeNull, scriptID, &moduleDependenciesLabel, &deplist_id);
	if (noErr == err) {
#if useLog
		fprintf(stderr, "Found __module_dependencies__\n");
#endif
		err = OSACoerceToDesc(component, deplist_id, typeWildCard, kOSAModeNull, dependencies);
		goto bail;
	}
#if useLog
	fprintf(stderr, "Not Found __module_dependencies__\n");
#endif	
	err = OSAGetPropertyNames(component, kOSAModeNull, scriptID, &property_names);
	if (err != noErr) {
		fprintf(stderr, "Failed to OSAGetPropertyName in extractDependencies\n");
		goto bail;
	}
#if useLog
	showAEDesc(&property_names);
#endif	
	long count = 0;
	err = AECountItems(&property_names, &count);
	if (err != noErr) goto bail;
	
	AEKeyword a_keyword;
	for (long n = 1; n <= count; n++) {
		AEDesc a_pname;
		AECreateDesc(typeNull, NULL, 0, &a_pname);
		OSAID prop_value_id = kOSANullScript;
		AEDesc dep_info;
		AECreateDesc(typeNull, NULL, 0, &dep_info);
		AEDesc prop_desc;
		AECreateDesc(typeNull, NULL, 0, &prop_desc);
        AEDesc modspec_desc;
        AECreateDesc(typeNull, NULL, 0, &modspec_desc);
        
		err = AEGetNthDesc(&property_names, n, typeWildCard, &a_keyword, &a_pname);
		if (err != noErr) goto loopbail;
#if useLog
		showAEDesc(&a_pname);
#endif		
		if (typeType == a_pname.descriptorType) {
            OSType type_data;
            err = AEGetDescData(&a_pname, &type_data, sizeof(type_data));
            if (noErr != err) goto loopbail;
            if (type_data != 'pimr') {
                // if a_pname is not user defined property, skip
                goto loopbail;
            }
            err = OSAGetProperty(component, kOSAModeNull, scriptID, &a_pname, &reqitems_id);
            if (noErr != err) {
                fprintf(stderr, "Failed to OSAGetProperty for requested import items with error %d\n", err);
                goto loopbail;
            }
            err = OSACoerceToDesc(component, reqitems_id, typeWildCard, kOSAModeNull, &reqitems_desc);
            if (noErr != err) {
                fprintf(stderr, "Failed to OSACoerceToDesc for requested import items with error %d\n", err);
                goto loopbail;
            }
            reqested_items = [[NSAppleEventDescriptor alloc] initWithAEDescNoCopy:&reqitems_desc];
            goto loopbail;
		}
		err = OSAGetProperty(component, kOSAModeNull, scriptID, &a_pname, &prop_value_id);	
		if (noErr != err) { 
			fprintf(stderr, "Failed to OSAGetProperty in extractDependencies with error %d\n", err);
			goto loopbail;
		}
		long is_script;
		err = OSAGetScriptInfo(component, prop_value_id, kOSAScriptIsTypeScriptContext, &is_script);			
		if (err != noErr) goto loopbail;
		AEBuildError ae_err;
		if (is_script) {
            goto loopbail;
		} else {
			err = OSACoerceToDesc(component, prop_value_id, typeWildCard, kOSAModeNull, &prop_desc);
			if (err != noErr) goto loopbail;
            DescType type_code;
            Size data_size = 0;
            Boolean ismodule = false;
            switch (prop_desc.descriptorType) {
                case typeObjectSpecifier:
                    err = ConvertToModuleSpecifier(&prop_desc, &modspec_desc,
                                                   reqested_items, &ismodule);
                    if (!ismodule) goto loopbail;
                    err = AEBuildDesc(&dep_info, &ae_err, "DpIf{pnam:@, MoSp:@, fmUs:@}",&a_pname, &modspec_desc, &true_desc);
                    break;
                case typeModuleSpecifier:
                    err = AESizeOfKeyDesc(&prop_desc, keyAEName, &type_code, &data_size);
                    if (!data_size) {
                        err = AEPutKeyDesc(&prop_desc, keyAEName, &a_pname);
                        if (noErr != err) goto loopbail;
                    }
                    err = AEBuildDesc(&dep_info, &ae_err, "DpIf{pnam:@, MoSp:@}",&a_pname, &prop_desc);
                    break;
                default:
                    goto loopbail;
			}

		}
	
        if (err != noErr) goto loopbail;
		AEPutDesc(dependencies, 0, &dep_info);
		
	loopbail:
		AEDisposeDesc(&a_pname);
		AEDisposeDesc(&prop_desc);
		AEDisposeDesc(&dep_info);
        AEDisposeDesc(&modspec_desc);
		OSADispose(component, prop_value_id);
		if (noErr != err) goto bail;
	}
bail:
	AEDisposeDesc(&property_names);
	AEDisposeDesc(&moduleSpecLabel);
	AEDisposeDesc(&moduleDependenciesLabel);
    AEDisposeDesc(&true_desc);
    OSADispose(component, reqitems_id);
    return err;
}

