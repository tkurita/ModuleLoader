#include "findModule.h"
#include "AEUtils.h"
#include "ModuleLoaderConstants.h"

#define useLog 0

static CFArrayRef MODULE_PATHS = NULL;
static char *SUFFIXES[4] = {"", ".scptd", ".scpt", ".app"};

void setAdditionalModulePaths(CFArrayRef array)
{
	if (MODULE_PATHS) 
		CFRelease(MODULE_PATHS);

	MODULE_PATHS = array;	
	CFPreferencesSetAppValue(MODULE_PATHS_KEY, 
							MODULE_PATHS,
							BUNDLE_ID);
	CFPreferencesAppSynchronize(BUNDLE_ID);
	if (MODULE_PATHS) CFRetain(MODULE_PATHS);
}

CFArrayRef additionalModulePaths()
{
	if (MODULE_PATHS) goto bail;
	MODULE_PATHS = CFPreferencesCopyAppValue(MODULE_PATHS_KEY, BUNDLE_ID);
#if useLog	
	CFShow(MODULE_PATHS);
#endif
bail:
	return MODULE_PATHS;
}

CFArrayRef copyDefaultModulePaths()
{
	OSErr err;
	CFMutableArrayRef paths = CFArrayCreateMutable(NULL, 3, &kCFTypeArrayCallBacks);
	FSRef scripts_folder;
	FSRef modules_folder;
	int domains[3] = {kUserDomain, kLocalDomain, kNetworkDomain};
	for (int n=0; n < 3; n++) {
		err = FSFindFolder(domains[n], kScriptsFolderType, false, &scripts_folder);
		if (noErr != err) continue;
		err = FSMakeFSRefChild(&scripts_folder, CFSTR("Modules"), &modules_folder);
		if (noErr != err) continue;
		CFURLRef url = CFURLCreateFromFSRef(NULL, &modules_folder);
		CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		CFArrayAppendValue(paths, path);
		CFRelease(url);
		CFRelease(path);
	}
	return paths;
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

OSErr scanFolder(FSRef *container_ref, CFStringRef module_name, FSRef *outRef, Boolean searchSubFolders)
{
	OSErr err = noErr;
	FSIterator itor = NULL;
	FSCatalogInfo cat_info;
	ItemCount ict;
	FSRef fsref;
	CFMutableArrayRef folder_array = NULL;
#if useLog
	UInt8 container_path[PATH_MAX];
	FSRefMakePath(container_ref, container_path, PATH_MAX);
	fprintf(stderr, "start scanFolder for : %s \n", (char *)container_path);
#endif	
	err = pickupModuleAtFolder(container_ref, module_name, outRef);
	if (noErr == err) {
		goto bail;
	} else if (!searchSubFolders) {
		err = kModuleIsNotFound;
		goto bail;
	}
	
	err = FSOpenIterator(container_ref, kFSIterateFlat, &itor);
	if (err != noErr) {
		fprintf(stderr, "Failed to FSOpenIterator with error : %d\n", err);
		goto bail;
	}

	folder_array = CFArrayCreateMutable(NULL, 5, &kCFTypeArrayCallBacks);
	FSCatalogInfoBitmap whichInfo = kFSCatInfoFinderInfo|kFSCatInfoNodeFlags;
	while (!FSGetCatalogInfoBulk(itor, 1, &ict, NULL, whichInfo, &cat_info, &fsref, NULL, NULL)) {
		if (kIsAlias & ((FileInfo *)(&cat_info.finderInfo))->finderFlags) {
			Boolean targetIsFolder;
			Boolean wasAliased;
			err = FSResolveAliasFile(&fsref, true, &targetIsFolder, &wasAliased);
			if (noErr != err) {
				fprintf(stderr, "Failed to FSResolveAliasFile with error : %d\n", err);
				continue;
			}

			err = FSGetCatalogInfo(&fsref, whichInfo, &cat_info, NULL, NULL, NULL);
			if (noErr != err) {
				fprintf(stderr, "Failed to FSGetCatalogInfo with error : %d\n", (int)err);
				continue;
			}
		}
		
		if (0 != (cat_info.nodeFlags & kFSNodeIsDirectoryMask)) {
			LSItemInfoRecord iteminfo;
			err = LSCopyItemInfoForRef(&fsref, kLSRequestBasicFlagsOnly, &iteminfo);
			if (noErr == err) {
				if (! (iteminfo.flags & kLSItemInfoIsPackage)) {
#if useLog
					UInt8 subfolder_path[PATH_MAX];
					FSRefMakePath(&fsref, subfolder_path, PATH_MAX);
					fprintf(stderr, "subfolder : %s \n", (char *)subfolder_path);
#endif					
					err = scanFolder(&fsref, module_name, outRef, searchSubFolders);
					if (err == noErr) goto bail;
				}
			} else {
				fprintf(stderr, "Faild to LSCopyItemInfoForRef with error : %d\n", (int)err);
			}
		}
	}

	err = kModuleIsNotFound;
	goto bail;

bail:
	safeRelease(folder_array);
	if (itor) FSCloseIterator(itor);
#if useLog	
	fprintf(stderr, "scanFolder will end with err :%d.\n", err);
#endif
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
	OSErr err = FSMakeFSRefUnicode(parentRef, length, name, kCFStringEncodingUnicode, newRef);	
	if (buffer) free(buffer);
	return err;
}

OSErr pickupModuleAtFolder(FSRef *container_ref, CFStringRef module_name, FSRef* module_ref)
{
	OSErr err = noErr;
	CFMutableStringRef filename = NULL;
	FSCatalogInfoBitmap whichinfo = kFSCatInfoFinderInfo|kFSCatInfoNodeFlags;
	FSCatalogInfo cat_info;
	for (int n = 0; n < 4; n++) {
		filename = CFStringCreateMutableCopy(NULL, 0, module_name);
		CFStringAppendCString(filename, SUFFIXES[n], kCFStringEncodingUTF8);
		err = FSMakeFSRefChild(container_ref, filename, module_ref);
		if (noErr == err) {
			err = FSGetCatalogInfo(module_ref, whichinfo, &cat_info, NULL, NULL, NULL);
			if (noErr != err) {
				fputs("Failed to FSGetCatalogInfo", stderr);
				break;
			}
			if (isScript(module_ref, &cat_info)) break;
		}
		
		CFRelease(filename);filename = NULL;
	}
	
bail:
	safeRelease(filename);
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
		//return scanFolder(&parentdir_ref, CFArrayGetValueAtIndex(path_comps, n), module_ref ,false);
		return pickupModuleAtFolder(&parentdir_ref, CFArrayGetValueAtIndex(path_comps, n), module_ref);
		
	}
	return kModuleIsNotFound;
}


OSErr findModule(CFStringRef moduleName, CFArrayRef additionalPaths, Boolean ingoreDefaultPaths,
				 FSRef *moduleRef, CFMutableArrayRef* searcedPaths)
{
	OSErr err = noErr;
	CFRange colon_range  = CFStringFind(moduleName, CFSTR(":"), 0);
	OSErr (*findModuleAtFolder)(FSRef *container_ref, CFTypeRef module_spec, FSRef* moduleRef);
	CFTypeRef module_spec = NULL;
	if (colon_range.location == kCFNotFound) {
		findModuleAtFolder = findModuleWithName;
		module_spec = moduleName;
		CFRetain(module_spec);
	} else {
		findModuleAtFolder = findModuleWithSubPath;
		module_spec = CFStringCreateArrayBySeparatingStrings(NULL, moduleName, CFSTR(":"));
	}
	
	FSRef scripts_folder;
	FSRef modules_folder;
	CFMutableArrayRef path_list = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
	if (additionalPaths) {
		CFArrayAppendArray(path_list, additionalPaths, 
						   CFRangeMake(0, CFArrayGetCount(additionalPaths)));		
	}
	
	if (!ingoreDefaultPaths) {
		CFArrayRef tmp_pathlist = additionalModulePaths();
		if (tmp_pathlist) {
			CFArrayAppendArray(path_list, tmp_pathlist, 
							   CFRangeMake(0, CFArrayGetCount(tmp_pathlist)));
		}
	}

	for (int n=0; n < CFArrayGetCount(path_list); n++) {
		CFStringRef cf_path = CFArrayGetValueAtIndex(path_list, n);
		CFIndex buff_size = CFStringGetMaximumSizeOfFileSystemRepresentation(cf_path);
		char *buffer = malloc(buff_size);
		CFStringGetFileSystemRepresentation(cf_path, buffer, buff_size);
		Boolean isDirectory;
		err = FSPathMakeRef((const UInt8 *)buffer, &modules_folder, &isDirectory);
		if (err == noErr) {
			err = findModuleAtFolder(&modules_folder, module_spec, moduleRef);
		}
		free(buffer);
		if (err == noErr) {
			goto bail;
		}
	}
	
	*searcedPaths = path_list;
	
	if (ingoreDefaultPaths) goto bail;
	
	int domains[3] = {kUserDomain, kLocalDomain, kNetworkDomain};
	for (int n=0; n < 3; n++) {
		err = FSFindFolder(domains[n], kScriptsFolderType, false, &scripts_folder);
		if (noErr != err) continue;
		err = FSMakeFSRefChild(&scripts_folder, CFSTR("Modules"), &modules_folder);
		if (noErr != err) continue;
		err = findModuleAtFolder(&modules_folder, module_spec, moduleRef);
		CFURLRef modulefolder_url = CFURLCreateFromFSRef(NULL, &modules_folder);
		CFStringRef modulefolder_path = CFURLCopyFileSystemPath(modulefolder_url, kCFURLPOSIXPathStyle);
		CFArrayAppendValue(*searcedPaths, modulefolder_path);
		CFRelease(modulefolder_url);
		CFRelease(modulefolder_path);
		if (err == noErr) {
#if useLog
			fprintf(stderr, "Module Found\n");
#endif
			break;
		}
		
	}
bail:
	safeRelease(module_spec);
	return err;
}
