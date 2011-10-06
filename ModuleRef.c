#include <CoreFoundation/CoreFoundation.h>
#include "AEUtils.h"
#include "ModuleCondition.h"

#define useLog 0

CFStringRef ModuleRefGetVersion(ModuleRef *module_ref)
{
	if (module_ref->version) {
		return module_ref->version;
	}
	if (! module_ref->is_package) return NULL;
	
	CFURLRef url = CFURLCreateFromFSRef(NULL, &module_ref->fsref);	
#if useLog
	fputs("In ModuleRefGetVersion", stderr);
	CFShow(url);
#endif		
	CFDictionaryRef dict = CFBundleCopyInfoDictionaryForURL(url);
	CFStringRef ver = CFDictionaryGetValue(dict, CFSTR("CFBundleShortVersionString"));
	if (!ver) ver = CFDictionaryGetValue(dict, CFSTR("CFBundleVersion"));
	if (ver) module_ref->version = CFRetain(ver);
	CFRelease(url);
	CFRelease(dict);
	return ver;
}


Boolean isScript(TXFileRef txfile)
{
	Boolean result = false;
	OSStatus err = noErr;
	CFStringRef uti = nil;
	
	Boolean wasAliased = false;
	
	err = TXFileResolveAlias(txfile, &wasAliased);
	if (noErr != err) {
		fprintf(stderr, "Failed to TXFileResolveAlias with error : %d\n", err);
		goto bail;
	}
	
	if (TXFileIsDirectory(txfile, (OSErr *)&err) ){
		LSItemInfoRecord *iteminfo = TXFileGetLSItemInfo(txfile, 0, (OSErr *)&err);
		if (noErr != err) {
			fprintf(stderr, "Failed to LSCopyItemInfoForRef with error : %d\n", (int)err);
			goto bail;
		}			
		
		if (! (iteminfo->flags & kLSItemInfoIsPackage)) {
			goto bail;
		} 
		if ((iteminfo->flags & kLSItemInfoIsApplication) ) {
#if useLog
			CFStringRef creator = UTCreateStringForOSType(iteminfo->creator);
			CFShow(creator);
#endif
			result = ((iteminfo->creator == 'aplt') || (iteminfo->creator == 'dplt')); 
			// if true, AppleScript Application
			goto bail;
		}
		
		if (kCFCompareEqualTo ==  CFStringCompare(iteminfo->extension, CFSTR("scptd"), 0)) {
			result = true;
			goto bail;
		}
	} else {
		if (noErr != err) 
			fprintf(stderr, "Faild to TXFileIsDirectory with error : %d\n", err);
	}
	
	err = LSCopyItemAttribute(TXFileGetFSRefPtr(txfile), kLSRolesAll, kLSItemContentType, (CFTypeRef *)&uti );
	if (noErr != err) {
		fprintf(stderr, "Failed to LSCopyItemAttribute with error : %d\n", (int)err);
		goto bail;
	}
	
	if (result = UTTypeConformsTo(uti, CFSTR("com.apple.applescript.script"))) {
		goto bail;
	}
	
	if (result = UTTypeConformsTo(uti, CFSTR("com.apple.applescript.text"))) {
		goto bail;
	}
bail:
	safeRelease(uti);
	return result;
}

ModuleRef *ModuleRefCreate(TXFileRef txfile)
{
	ModuleRef *module_ref = NULL;
	if (!isScript(txfile)) return NULL;
	module_ref = malloc(sizeof(ModuleRef));
	module_ref->fsref = *TXFileGetFSRefPtr(txfile);
	module_ref->version = NULL;
	module_ref->name = NULL;
	OSErr err = noErr;
	module_ref->is_package = TXFileIsPackage(txfile, &err);
	if (noErr != err) {
		fprintf(stderr, "Failed to TXFileIsPackage with error : %d\n", err);
	}
bail:
	return module_ref;
}

void ShowModuleRef(ModuleRef *module_ref)
{
	CFShow(CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
					CFSTR("name : %@, version : %@"), module_ref->name, module_ref->version));
}

ModuleRef *ModuleRefCreateWithCondition(TXFileRef txfile, CFStringRef filename, ModuleCondition *module_condition)
{
	CFArrayRef array = ModuleConditionParseName(module_condition, filename);
#if useLog
	CFShow(filename);
	CFShow(array);
#endif
	if (!array) return NULL;
	if (!isScript(txfile)) return NULL;
	ModuleRef *module_ref = malloc(sizeof(ModuleRef));
	module_ref->fsref = *TXFileGetFSRefPtr(txfile);
	OSErr err = noErr;
	module_ref->is_package = TXFileIsPackage(txfile, &err);
	if (noErr != err)
		fprintf(stderr, "Failed to TXFileIsPackage with error : %d\n", err);
	CFStringRef text = CFArrayGetValueAtIndex(array, 0);
	module_ref->name = CFRetain(text);
	CFStringRef version = CFArrayGetValueAtIndex(array, 2);
	if (CFStringGetLength(version)) {
		module_ref->version = CFRetain(version);
	} else {
		module_ref->version = NULL;
	}
	
	if (!ModuleConditionVersionIsSatisfied(module_condition, module_ref)) {
		ModuleRefFree(module_ref);
		module_ref = NULL;
	}
	
	CFRelease(array);
	return module_ref;	
}

void ModuleRefFree(ModuleRef *module_ref)
{
	if (!module_ref) return;
	safeRelease(module_ref->name);
	safeRelease(module_ref->version);
	free(module_ref);
}

CFComparisonResult ModuleRefCompareVersion(ModuleRef *module_ref1, ModuleRef *module_ref2)
{
	CFStringRef ver1 = ModuleRefGetVersion(module_ref1);
	CFStringRef ver2 = ModuleRefGetVersion(module_ref2);

	if (ver1 && !ver2) return kCFCompareGreaterThan;
	if (ver2 && !ver1) return kCFCompareLessThan;
	if (!ver1 && !ver2) return  kCFCompareEqualTo;
	return CFStringCompare(ver1, ver2, kCFCompareNumerically);
}