#include <CoreFoundation/CoreFoundation.h>
#include "ModuleCondition.h"

#include "AEUtils.h"
#define kIsPackageUnknown 2
CFStringRef ModuleRefGetVersion(ModuleRef *module_ref)
{
	if (module_ref->version) {
		return module_ref->version;
	}
	if (module_ref->is_package == kIsPackageUnknown) {
		LSItemInfoRecord iteminfo;
		OSStatus err = LSCopyItemInfoForRef(&module_ref->fsref, kLSRequestBasicFlagsOnly, &iteminfo);
		if (noErr != err) {
			fprintf(stderr, "Failed to LSCopyItemInfoForRef\n");
			return NULL;
		}
		if (iteminfo.flags & kLSItemInfoIsPackage) {
			module_ref->is_package = true;
		} else {
			module_ref->is_package = false;
		}
	}
	if (module_ref->is_package == false) return NULL;
	
	CFURLRef url = CFURLCreateFromFSRef(NULL, &module_ref->fsref);
	CFDictionaryRef dict = CFBundleCopyInfoDictionaryForURL(url);
	CFStringRef ver = CFDictionaryGetValue(dict, CFSTR("CFBundleShortVersionString"));
	if (!ver) ver = CFDictionaryGetValue(dict, CFSTR("CFBundleVersion"));
	if (ver) CFRetain(ver);
	module_ref->version = ver;
	CFRelease(url);
	CFRelease(dict);
	return ver;
}


Boolean isScript(FSRef *fsref, FSCatalogInfo* cat_info)
{
	Boolean result = false;
	OSStatus err;
	LSItemInfoRecord iteminfo;
	iteminfo.extension = NULL;
	
	if (kIsAlias & ((FileInfo *)(&cat_info->finderInfo))->finderFlags) {
		Boolean targetIsFolder;
		Boolean wasAliased;
		err = FSResolveAliasFile(fsref, true, &targetIsFolder, &wasAliased);
		if (err != noErr) {
			fprintf(stderr, "Failed to FSResolveAliasFile with error : %d\n", (int)err);
			goto bail;
		}
		FSCatalogInfoBitmap whichInfo = kFSCatInfoFinderInfo|kFSCatInfoNodeFlags;
		err = FSGetCatalogInfo(fsref, whichInfo, cat_info, NULL, NULL, NULL);
		if (err != noErr) {
			fprintf(stderr, "Failed to FSGetCatalogInfo with error : %d\n", (int)err);
			goto bail;;
		}
	}
	
	if (0 != (cat_info->nodeFlags & kFSNodeIsDirectoryMask)) { // is directory
		err = LSCopyItemInfoForRef(fsref, kLSRequestAllInfo & ~kLSRequestIconAndKind, 
								   &iteminfo);
		if (err != noErr) {
			fprintf(stderr, "Failed to LSCopyItemInfoForRef with error : %d\n", (int)err);
			goto bail;
		}
		
		if (! (iteminfo.flags & kLSItemInfoIsPackage)) {
			goto bail;
		} 
		if ((iteminfo.flags & kLSItemInfoIsApplication) ) {
#if useLog
			CFStringRef creator = UTCreateStringForOSType(iteminfo.creator);
			CFShow(creator);
#endif
			result = ((iteminfo.creator == 'aplt') || (iteminfo.creator == 'dplt'));
			goto bail;
		}
		
		if (kCFCompareEqualTo ==  CFStringCompare(iteminfo.extension, CFSTR("scptd"), 0)) {
			result = true;
			goto bail;
		}
	}
	
	if (((FileInfo *)(&cat_info->finderInfo))->fileType == 'osas' ) {
		result = true;
		goto bail;
	}
	
	err = LSCopyItemInfoForRef(fsref, kLSRequestAllInfo & ~kLSRequestIconAndKind, 
							   &iteminfo);
	if (err != noErr) {
		fprintf(stderr, "Failed to LSCopyItemInfoForRef with error : %d\n", (int)err);
		goto bail;
	}
	
	if (kCFCompareEqualTo ==  CFStringCompare(iteminfo.extension, CFSTR("scpt"), 0)) {
		result = true;
		goto bail;
	}
	
bail:
	safeRelease(iteminfo.extension);
	return result;
}

ModuleRef *ModuleRefCreate(FSRef fsref, FSCatalogInfo *cat_info)
{
	if (!isScript(&fsref, cat_info)) return NULL;
	ModuleRef *module_ref = malloc(sizeof(ModuleRef));
	module_ref->fsref = fsref;
	module_ref->version = NULL;
	module_ref->name = NULL;
	module_ref->is_package = kIsPackageUnknown;
	return module_ref;
}

ModuleRef *ModuleRefCreateWithCondition(FSRef *fsref, FSCatalogInfo *cat_info, CFStringRef filename, Boolean is_package,
										ModuleCondition *module_condition)
{
	CFArrayRef array = ModuleConditionParseName(module_condition, filename);
	if (!array) return NULL;
	if (!isScript(fsref, cat_info)) return NULL;
	ModuleRef *module_ref = malloc(sizeof(ModuleRef));
	module_ref->fsref = *fsref;
	module_ref->is_package = is_package;
	CFStringRef text = CFArrayGetValueAtIndex(array, 0);
	CFRetain(text);
	module_ref->name = text;
	CFStringRef version = CFArrayGetValueAtIndex(array, 1);
	if (CFStringGetLength(version)) {
		CFRetain(version);
		module_ref->version = version;
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