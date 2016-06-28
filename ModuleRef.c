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
	CFStringRef uti = NULL;
	
    CFErrorRef error = NULL;
    if (! TXFileResolveAlias(txfile, &error) ) {
        if (error) {
            CFShow(error);
        }
        goto bail;
    }
    uti = TXFileCopyTypeIdentifier(txfile, &error);
    if (error) {
        CFShow(error);
        goto bail;
    }
    if ( (result = UTTypeConformsTo(uti, CFSTR("com.apple.applescript.script-bundle"))) ) {
        result = true;
        goto bail;
    }
    
    if ( (result = UTTypeConformsTo(uti, CFSTR("com.apple.applescript.script"))) ) {
        result = true;
        goto bail;
    }
    
    if ( (result = UTTypeConformsTo(uti, CFSTR("com.apple.applescript.text"))) ) {
        result = true;
        goto bail;
    }
    
    if ( (result = UTTypeConformsTo(uti, CFSTR("com.apple.application-bundle"))) ) {
        CFBundleRef bundle = CFBundleCreate(kCFAllocatorDefault, TXFileGetURL(txfile));
        CFDictionaryRef dict = CFBundleGetInfoDictionary(bundle);
        CFStringRef creator = CFDictionaryGetValue(dict, CFSTR("CFBundleSignature"));

        if ((kCFCompareEqualTo ==  CFStringCompare(creator, CFSTR("dplt"), 0))
                || (kCFCompareEqualTo ==  CFStringCompare(creator, CFSTR("aplt"), 0))) {
            result = true;
        }
        CFRelease(bundle);
        goto bail;
    }

bail:
    safeRelease(error);
	safeRelease(uti);
	return result;
}

ModuleRef *ModuleRefCreate(TXFileRef txfile)
{
	ModuleRef *module_ref = NULL;
	if (!isScript(txfile)) return NULL;
	module_ref = malloc(sizeof(ModuleRef));
	module_ref->fsref = *TXFileGetFSRefPtr(txfile);
    module_ref->url = TXFileCopyURL(txfile);
	module_ref->version = NULL;
	module_ref->name = NULL;
    CFErrorRef error = NULL;
    module_ref->is_package = TXFileIsPackage(txfile, &error);
    if (error) CFShow(error);
bail:
	return module_ref;
}

void ShowModuleRef(ModuleRef *module_ref)
{
	CFShow(CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
					CFSTR("name : %@, version : %@"), module_ref->name, module_ref->version));
}

ModuleRef *ModuleRefCreateWithCondition(TXFileRef txfile, ModuleCondition *module_condition)
{
    CFErrorRef error = NULL;
    CFStringRef filename = NULL;
    CFArrayRef array = NULL;
    filename = TXFileCopyAttribute(txfile, kCFURLNameKey, &error);
    if (error) {
        CFShow(error); goto bail;
    }
    array = ModuleConditionParseName(module_condition, filename);
#if useLog
	CFShow(filename);
	CFShow(array);
#endif
	if (!array) return NULL;
	if (!isScript(txfile)) return NULL;
	ModuleRef *module_ref = malloc(sizeof(ModuleRef));
	module_ref->fsref = *TXFileGetFSRefPtr(txfile);
	module_ref->is_package = TXFileIsPackage(txfile, &error);
    if (error) {
        CFShow(error); goto bail;
    }
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
bail:
    safeRelease(array);
    safeRelease(error);
    safeRelease(filename);
	return module_ref;	
}

void ModuleRefFree(ModuleRef *module_ref)
{
	if (!module_ref) return;
	safeRelease(module_ref->name);
	safeRelease(module_ref->version);
    safeRelease(module_ref->url);
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