#include "ModuleLoaderConstants.h"
#include "AEUtils.h"
#include "findModule.h"

#define useLog 0

static CFArrayRef MODULE_PATHS = NULL;
static char *SUFFIXES[5] = {"", ".scptd", ".scpt", ".app", ".applescript"};
static unsigned int NSUFFIXES = 5;

void setAdditionalModulePaths(CFArrayRef array)
{
	if (MODULE_PATHS) 
		CFRelease(MODULE_PATHS);

	MODULE_PATHS = array;	
	CFPreferencesSetAppValue(MODULE_PATHS_KEY, 
							MODULE_PATHS,
							BUNDLE_ID);
	CFPreferencesAppSynchronize(BUNDLE_ID);
	if (MODULE_PATHS) MODULE_PATHS = CFRetain(MODULE_PATHS);
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



OSErr scanFolder(FSRef *container_ref, ModuleCondition *module_condition, FSRef *outRef, Boolean searchSubFolders)
{
	OSErr err = noErr;
	FSIterator itor = NULL;
	FSCatalogInfo cat_info;
	ItemCount ict;
	FSRef fsref;
	CFStringRef fname = NULL;
	CFMutableArrayRef subfolders = NULL;
	ModuleRef *module_ref_candidate = NULL;
	ModuleRef *module_ref = NULL;
#if useLog
	UInt8 container_path[PATH_MAX];
	FSRefMakePath(container_ref, container_path, PATH_MAX);
	fprintf(stderr, "start scanFolder for : %s \n", (char *)container_path);
#endif	
	err = pickupModuleAtFolder(container_ref, module_condition, outRef);
	if (noErr == err) {
		goto bail;
	} 
		
	/* start to search subfolders */
	if (searchSubFolders) subfolders = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
	err = FSOpenIterator(container_ref, kFSIterateFlat, &itor);
	if (err != noErr) {
		fprintf(stderr, "Failed to FSOpenIterator with error : %d\n", err);
		goto bail;
	}
	HFSUniStr255 hfs_filename[1];
	FSCatalogInfoBitmap whichInfo = kFSCatInfoFinderInfo|kFSCatInfoNodeFlags;
	while (!FSGetCatalogInfoBulk(itor, 1, &ict, NULL, whichInfo, &cat_info, &fsref, NULL, &hfs_filename[0])) {
		Boolean is_package = false;
		fname = CFStringCreateWithCharacters(kCFAllocatorDefault, hfs_filename[0].unicode, hfs_filename[0].length);
#if useLog
		CFShow(fname);
#endif		
		// resolving alias file
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
				
		Boolean is_folder = false;
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
					is_folder = true;
				} else {
					is_package = true;
				}
			} else {
				fprintf(stderr, "Faild to LSCopyItemInfoForRef with error : %d\n", (int)err);
			}
		}
		
		if (is_folder) {
			if (searchSubFolders) {
				CFArrayAppendValue(subfolders, CFURLCreateFromFSRef(kCFAllocatorDefault, &fsref));
			}
		} else {
			module_ref = ModuleRefCreateWithCondition(&fsref, &cat_info, fname, is_package, module_condition);
			if (module_ref) {
#if useLog
				ShowModuleRef(module_ref);
#endif
				if (module_ref_candidate) {
					CFComparisonResult cr = ModuleRefCompareVersion(module_ref, module_ref_candidate);
					if (cr ==  kCFCompareGreaterThan) {
						ModuleRefFree(module_ref_candidate);
						module_ref_candidate = module_ref;
					} else {
						ModuleRefFree(module_ref);
					}
				} else {
					module_ref_candidate = module_ref;
				}
			}		
		}
		CFRelease(fname); fname = NULL;
	}
	
	if (module_ref_candidate) {
		*outRef = module_ref_candidate->fsref;
		ModuleRefFree(module_ref_candidate);
		goto bail;
	}
	
	if (searchSubFolders) {
		for (CFIndex n = 0; n < CFArrayGetCount(subfolders); n++) {
			FSRef subfolder_ref;
			CFURLGetFSRef(CFArrayGetValueAtIndex(subfolders, n), &subfolder_ref);
			err = scanFolder(&subfolder_ref, module_condition, outRef, searchSubFolders);
			if (err == noErr) goto bail;
		}
	} 
	err = kModuleIsNotFound;
bail:
	if (itor) FSCloseIterator(itor);
#if useLog	
	fprintf(stderr, "scanFolder will end with err :%d.\n", err);
#endif
	safeRelease(subfolders);
	safeRelease(fname);
	return err;
}

OSErr findModuleWithName(FSRef *container_ref, ModuleCondition *module_condition, FSRef* module_ref)
{
	return scanFolder(container_ref, module_condition, module_ref ,true);
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

OSErr pickupModuleAtFolder(FSRef *container_ref, ModuleCondition *module_condition, FSRef *out_module_ref)
{
	OSErr err = noErr;
	CFMutableStringRef filename = NULL;
	ModuleRef *module_ref = NULL;
	FSCatalogInfoBitmap whichinfo = kFSCatInfoFinderInfo|kFSCatInfoNodeFlags;
	FSCatalogInfo cat_info;
	CFStringRef module_name = module_condition->name;
	FSRef buffer_ref;
	for (int n = 0; n < NSUFFIXES; n++) {
		filename = CFStringCreateMutableCopy(NULL, 0, module_name);
		CFStringAppendCString(filename, SUFFIXES[n], kCFStringEncodingUTF8);
		err = FSMakeFSRefChild(container_ref, filename, &buffer_ref);
		if (noErr == err) {
			err = FSGetCatalogInfo(&buffer_ref, whichinfo, &cat_info, NULL, NULL, NULL);
			if (noErr != err) {
				fputs("Failed to FSGetCatalogInfo", stderr);
				break;
			}
			module_ref = ModuleRefCreate(&buffer_ref, &cat_info);
			if (module_ref) {
				if (ModuleConditionVersionIsSatisfied(module_condition, module_ref)) break;
			}
			ModuleRefFree(module_ref);module_ref = NULL;
			err = kModuleIsNotFound;
		}
		
		CFRelease(filename);filename = NULL;
	}
	
	if (module_ref) *out_module_ref=module_ref->fsref;
bail:
	safeRelease(filename);
	ModuleRefFree(module_ref);
	return err;
}

OSErr findModuleWithSubPath(FSRef *container_ref, ModuleCondition *module_condition, FSRef* module_ref)
{
	OSErr err;
	Boolean is_exists = true;
	FSRef parentdir_ref = *container_ref;
	CFArrayRef path_comps = module_condition->subpath;
	for (CFIndex n=0; n<(CFArrayGetCount(path_comps) -1); n++) {
		CFStringRef path_elem = CFArrayGetValueAtIndex(path_comps, n);
		if (!CFStringGetLength(path_elem)) continue;
		FSRef subdir_ref;
		err = FSMakeFSRefChild(&parentdir_ref, path_elem, &subdir_ref);
		is_exists = (err == noErr);
		if (!is_exists) break;
		parentdir_ref = subdir_ref;
	}
	if (is_exists) {
		err = pickupModuleAtFolder(&parentdir_ref, module_condition, module_ref);
		if (err != noErr) {
			err = scanFolder(&parentdir_ref, module_condition, module_ref, false);
		}
		if (err == noErr) return err;
	}
	return kModuleIsNotFound;
}


OSErr findModule(ModuleCondition *module_condition, CFArrayRef additionalPaths, Boolean ingoreDefaultPaths,
				 FSRef *moduleRef, CFMutableArrayRef* searchedPaths)
{
	OSErr err = noErr;
	OSErr (*findModuleAtFolder)(FSRef *container_ref, ModuleCondition *module_condition, FSRef* moduleRef);
	if (ModuleConditionHasSubpath(module_condition)) {
		findModuleAtFolder = findModuleWithSubPath;
	} else {
		findModuleAtFolder = findModuleWithName;
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
			err = findModuleAtFolder(&modules_folder, module_condition, moduleRef);
		}
		free(buffer);
		if (err == noErr) {
			goto bail;
		}
	}
	
	*searchedPaths = path_list;
	
	if (ingoreDefaultPaths) goto bail;
	
	int domains[3] = {kUserDomain, kLocalDomain, kNetworkDomain};
	for (int n=0; n < 3; n++) {
		err = FSFindFolder(domains[n], kScriptsFolderType, false, &scripts_folder);
		if (noErr != err) continue;
		err = FSMakeFSRefChild(&scripts_folder, CFSTR("Modules"), &modules_folder);
		if (noErr != err) continue;
		err = findModuleAtFolder(&modules_folder, module_condition, moduleRef);
		CFURLRef modulefolder_url = CFURLCreateFromFSRef(NULL, &modules_folder);
		CFStringRef modulefolder_path = CFURLCopyFileSystemPath(modulefolder_url, kCFURLPOSIXPathStyle);
		CFArrayAppendValue(*searchedPaths, modulefolder_path);
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
	return err;
}
