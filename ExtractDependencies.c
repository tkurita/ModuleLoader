#include <Carbon/Carbon.h>
#include "ExtractDependencies.h"
#include "ModuleLoaderConstants.h"
#include "AEUtils.h"

OSErr extractDependencies(ComponentInstance component, OSAID scriptID, AEDescList *dependencies)
{
	OSErr err;
	
	AEDescList property_names;
	AECreateDesc(typeNull, NULL, 0, &property_names);	
	
	AEDesc moduleSpecLabel;
	char *a_label = "__module_specifier__";
	err = AECreateDesc(typeChar, a_label, strlen(a_label), &moduleSpecLabel);
	
	err = AECreateList(NULL, 0, false, dependencies);
	if (err != noErr) goto bail;

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
		showAEDesc(&a_pname);
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
	}
bail:
	AEDisposeDesc(&property_names);
	AEDisposeDesc(&moduleSpecLabel);
	return err;				
}

