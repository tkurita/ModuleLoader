#include "findModule.h"

#define useLog 1

#pragma mark for debug


void safeRelease(CFTypeRef theObj)
{
	if (theObj != NULL) {
		CFRelease(theObj);
	}
}

Boolean isScript(FSRef *fsref, FSCatalogInfo* cat_info)
{
	Boolean result = false;
	OSStatus err;
	LSItemInfoRecord iteminfo;
	if (0 != (cat_info->nodeFlags & kFSNodeIsDirectoryMask)) { // is directory
		err = LSCopyItemInfoForRef(fsref, kLSRequestAllInfo, &iteminfo);
		if (err != noErr) {
			fprintf(stderr, "Failed to LSCopyItemInfoForRef with error : %d\n", err);
			goto bail;
		}
			
		if (! (iteminfo.flags & kLSItemInfoIsPackage)) {
			goto bail;
		} 
		if ((iteminfo.flags & kLSItemInfoIsApplication) ) {
			CFStringRef creator = UTCreateStringForOSType(iteminfo.creator);
			CFShow(creator);
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
	}
	
bail:	
	return result;
}

OSErr scanFolder(FSRef *container_ref, CFStringRef module_name, FSRef *outRef, Boolean searchSubFolders)
{
	OSErr err = noErr;
	FSIterator itor;
	FSCatalogInfo cat_info;
	ItemCount ict;
	FSRef fsref;
	err = FSOpenIterator(container_ref, kFSIterateFlat, &itor);
	if (err != noErr) {
		fprintf(stderr, "Failed to FSOpenIterator with error : %d\n", err);
		goto bail;
	}

	CFStringRef basename = NULL;
	CFURLRef urlref = NULL;
	CFMutableArrayRef folder_array = CFArrayCreateMutable(NULL, 5, &kCFTypeArrayCallBacks);
	
	while (!FSGetCatalogInfoBulk(itor, 1, &ict, NULL, kFSCatInfoFinderInfo|kFSCatInfoNodeFlags, 
								 &cat_info, &fsref, NULL, NULL)) {
		/*
		urlref = CFURLCreateFromFSRef(NULL, &fsref);
		CFShow(urlref);
		*/
		if (kIsAlias & ((FileInfo *)(&cat_info.finderInfo))->finderFlags) {
			Boolean targetIsFolder;
			Boolean wasAliased;
			err = FSResolveAliasFile(&fsref, true, &targetIsFolder, &wasAliased);
			if (err != noErr) {
				fprintf(stderr, "Failed to FSResolveAliasFile with error : %d\n", err);
				continue;
			}
			urlref = CFURLCreateFromFSRef(NULL, &fsref);
			CFShow(urlref);
			err = FSGetCatalogInfo(&fsref, kFSCatInfoFinderInfo|kFSCatInfoNodeFlags, &cat_info, NULL, NULL, NULL);
			if (err != noErr) {
				fprintf(stderr, "Failed to FSGetCatalogInfo with error : %d\n", err);
				continue;
			}
		}
		
		if (!isScript(&fsref, &cat_info)) {
			if (0 != (cat_info.nodeFlags & kFSNodeIsDirectoryMask)) {
				CFDataRef data = CFDataCreate(NULL, (UInt8 *)&fsref, sizeof(fsref));
				CFArrayAppendValue(folder_array, data);
				CFRelease(data);
			}
			continue;
		}
		
		fprintf(stderr, "is script \n");
		urlref = CFURLCreateFromFSRef(NULL, &fsref);
		basename = CFURLCopyLastPathComponent(
									CFURLCreateCopyDeletingPathExtension(NULL, urlref));
		
		if (kCFCompareEqualTo == CFStringCompare(basename, module_name, kCFCompareCaseInsensitive)) {
			*outRef = fsref;
			goto found;
		}
		CFRelease(basename);
		basename = CFURLCopyLastPathComponent(urlref);
		if (kCFCompareEqualTo == CFStringCompare(basename, module_name, kCFCompareCaseInsensitive)) {
			*outRef = fsref;
			goto found;
		}
		CFRelease(urlref); urlref = NULL;
		CFRelease(basename); basename = NULL;
	}
	
	if (!searchSubFolders) goto bail;
	
	for (int n =0; n < CFArrayGetCount(folder_array); n++) {
		CFDataRef data = CFArrayGetValueAtIndex(folder_array, n);
		FSRef *dir_ref = (FSRef *)CFDataGetBytePtr(data);
		err = scanFolder(dir_ref, module_name, outRef, searchSubFolders);
		if (FSIsFSRefValid(outRef)) goto bail;
	}
found:
	safeRelease(urlref);
	safeRelease(basename);
bail:
	CFRelease(folder_array);
	FSCloseIterator(itor);
	return err;
}

OSErr findModuleWithName(FSRef *container_ref, CFTypeRef module_name, FSRef* module_ref)
{
	return scanFolder(container_ref, (CFStringRef)module_name, module_ref ,true);
}

OSErr FSMakeFSRefChild(FSRef *parentRef, CFStringRef childName, FSRef *newRef)
{
	UniCharCount length = CFStringGetLength(childName);
	const UniChar *name = CFStringGetCharactersPtr(childName);
	UniChar *buffer = NULL;
	if (! name) {
        buffer = malloc(length * sizeof(UniChar));
        CFStringGetCharacters(childName, CFRangeMake(0, length), buffer);
        name = buffer;
	}
	OSErr err = FSMakeFSRefUnicode(parentRef, length, name, kTextEncodingUnicodeDefault, newRef);	
	if (buffer) free(buffer);
	return err;
}

OSErr findModuleWithSubPath(FSRef *container_ref, CFTypeRef path_components, FSRef* module_ref)
{
	CFArrayRef path_comps = (CFArrayRef)path_components;
	OSErr err;
	Boolean is_exists = true;
	FSRef parentdir_ref = *container_ref;
	int n;
	for (n=0; n<(CFArrayGetCount(path_comps) -1); n++) {
		CFStringRef path_elem = CFArrayGetValueAtIndex(path_comps, n);
		if (!CFStringGetLength(path_elem)) continue;
		FSRef subdir_ref;
		err = FSMakeFSRefChild(&parentdir_ref, path_elem, &subdir_ref);
		is_exists = (err == noErr);
		if (!is_exists) break;
		parentdir_ref = subdir_ref;
	}
	if (is_exists) {
		return scanFolder(container_ref, CFArrayGetValueAtIndex(path_comps, n), module_ref ,false);
	}
	return noErr;
}

OSErr findModule(CFStringRef moduleName, FSRef *moduleRef)
{
	OSErr err = noErr;
	CFRange colon_range  = CFStringFind(moduleName, CFSTR(":"), 0);
	OSErr (*findModuleAtFolder)(FSRef *container_ref, CFTypeRef module_spec, FSRef* moduleRef);
	CFTypeRef module_spec;
	if (colon_range.location == kCFNotFound) {
		findModuleAtFolder = findModuleWithName;
		module_spec = moduleName;
	} else {
		findModuleAtFolder = findModuleWithSubPath;
		CFArrayRef path_comps = CFStringCreateArrayBySeparatingStrings(NULL, moduleName, CFSTR(":"));
		module_spec = path_comps;
	}
	
	FSRef scripts_folder;
	FSRef modules_folder;
	int domains[3] = {kUserDomain, kLocalDomain, kNetworkDomain};
	for (int n=0; n < 3; n++) {
		err = FSFindFolder (domains[n], kScriptsFolderType, false, &scripts_folder);
		if (noErr != err) continue;
		err = FSMakeFSRefChild(&scripts_folder, CFSTR("Modules"), &modules_folder);
		if (noErr != err) continue;
		err = findModuleAtFolder(&modules_folder, module_spec, moduleRef);
		if (FSIsFSRefValid(moduleRef)) {
			fprintf(stderr, "Module Found\n");
			break;
		}
	}
	return err;
}
