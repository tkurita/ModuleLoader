#include <Carbon/Carbon.h>
#include "ModuleLoaderConstants.h"
#include "ExtractDependencies.h"
#include "AEUtils.h"

#define useLog 1

const char *MODULE_SPEC_LABEL = "__module_specifier__";
const char *MODULE_DEPENDENCIES_LABEL = "__module_dependencies__";

OSErr extractDependencies(ComponentInstance component, OSAID scriptID, AEDescList *dependencies)
{
	OSErr err;
	
	AEDescList property_names;
	AECreateDesc(typeNull, NULL, 0, &property_names);	
	
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
		OSAID prop_value_id2 = kOSANullScript;
		AEDesc dep_info;
		AECreateDesc(typeNull, NULL, 0, &dep_info);
		AEDesc prop_desc;
		AECreateDesc(typeNull, NULL, 0, &prop_desc);
		/*
		CFStringRef pname_string  = NULL;
		CFMutableStringRef pname_string_mutable = NULL;
		AEDesc lpname;
		AECreateDesc(typeNull, NULL, 0, &lpname);
		*/
		
		err = AEGetNthDesc(&property_names, n, typeWildCard, &a_keyword, &a_pname);
		if (err != noErr) goto loopbail;
#if useLog
		showAEDesc(&a_pname);
#endif		
		if (typeType == a_pname.descriptorType) {
			// if a_pname is not user defined property, skip
			goto loopbail;
		}
		/*
		pname_string = CFStringCreateWithAEDesc(&a_pname, &err);
		if (noErr != err) goto loopbail;
		
		pname_string_mutable = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, pname_string);
		CFStringLowercase(pname_string_mutable, CFLocaleGetSystem());
		CFShow(pname_string_mutable);
		err = AEDescCreateWithCFString(pname_string_mutable, kCFStringEncodingUnicode, &lpname);
		if (noErr != err) goto loopbail;
		showAEDesc(&lpname);
		 */
		//err = OSAGetProperty(component, kOSAModeNull, scriptID, &lpname, &prop_value_id);	
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
			err = OSAGetProperty(component, kOSAModeNull, prop_value_id, &moduleSpecLabel, &prop_value_id2);
			if (noErr != err) {
				err = noErr;
				goto loopbail;
			}
			err = OSACoerceToDesc(component, prop_value_id2, typeWildCard, kOSAModeNull, &prop_desc);
			if (err != noErr) goto loopbail;			
		} else {
			err = OSACoerceToDesc(component, prop_value_id, typeWildCard, kOSAModeNull, &prop_desc);
			if (err != noErr) goto loopbail;
			if (prop_desc.descriptorType != typeModuleSpecifier) {
				goto loopbail;
			}
			DescType type_code;
			Size data_size = 0;
			err = AESizeOfKeyDesc (&prop_desc, keyAEName, &type_code, &data_size);
			if (!data_size) {
				err = AEPutKeyDesc (&prop_desc, keyAEName, &a_pname);
				if (noErr != err) goto loopbail;
			}
		}
	
		err = AEBuildDesc(&dep_info, &ae_err, "DpIf{pnam:@, MoSp:@}",&a_pname, &prop_desc);
		if (err != noErr) goto loopbail;
		AEPutDesc(dependencies, 0, &dep_info);
		
	loopbail:
		/*
		safeRelease(pname_string);
		safeRelease(pname_string_mutable);
		AEDisposeDesc(&lpname);
		 */
		AEDisposeDesc(&a_pname);
		AEDisposeDesc(&prop_desc);
		AEDisposeDesc(&dep_info);
		OSADispose(component, prop_value_id);
		OSADispose(component, prop_value_id2);
		if (noErr != err) goto bail;
	}
bail:
	AEDisposeDesc(&property_names);
	AEDisposeDesc(&moduleSpecLabel);
	return err;				
}

