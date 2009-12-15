#include <Carbon/Carbon.h>
#include "ModuleLoaderConstants.h"
#include "ExtractDependencies.h"
#include "AEUtils.h"

#define useLog 0

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
	if (err != noErr) goto bail;

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
		
		err = AEGetNthDesc(&property_names, n, typeWildCard, &a_keyword, &a_pname);
		if (err != noErr) goto loopbail;
		//showAEDesc(&a_pname);
		if (typeType == a_pname.descriptorType) {
			// if a_pname is not user defined property, skip
			goto loopbail;
		}
		err = OSAGetProperty(component, kOSAModeNull, scriptID, &a_pname, &prop_value_id);
		
		if (err != noErr) goto loopbail;

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
			long count;
			AECountItems(&prop_desc, &count);
			if (!count) {
				AEDisposeDesc(&prop_desc);
				err = AEBuildDesc(&prop_desc, &ae_err, "MoSp{pnam:@}",&a_pname);
				if (noErr != err) goto loopbail;
			}
		}
	
		err = AEBuildDesc(&dep_info, &ae_err, "DpIf{pnam:@, MoSp:@}",&a_pname, &prop_desc);
		if (err != noErr) goto loopbail;
		AEPutDesc(dependencies, 0, &dep_info);
		
	loopbail:
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

