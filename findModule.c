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
	CFMutableArrayRef paths = CFArrayCreateMutable(kCFAllocatorDefault, 3, &kCFTypeArrayCallBacks);
	FSRef scripts_folder;
	FSRef modules_folder;
	int domains[3] = {kUserDomain, kLocalDomain, kNetworkDomain};
	for (int n=0; n < 3; n++) {
		err = FSFindFolder(domains[n], kScriptsFolderType, false, &scripts_folder);
		if (noErr != err) continue;
		err = FSMakeFSRefChild(&scripts_folder, CFSTR("Modules"), &modules_folder);
		if (noErr != err) continue;
		CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, &modules_folder);
		CFStringRef path = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
		CFArrayAppendValue(paths, path);
		CFRelease(url);
		CFRelease(path);
	}
	return paths;
}



OSErr scanFolder(FSRef *container_ref, ModuleCondition *module_condition,  Boolean searchSubFolders, ModuleRef **outRef)
{
	OSErr err = noErr;
	FSIterator itor = NULL;
	
	ItemCount ict;
	CFStringRef fname = NULL;
	CFMutableArrayRef subfolders = NULL;
	ModuleRef *module_ref_candidate = NULL;
	ModuleRef *module_ref = NULL;
	TXFileRef txfile = NULL;
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
	txfile = TXFileCreate(kCFAllocatorDefault, NULL);
	FSRef *fsref_p = TXFileGetFSRefPtr(txfile);
	FSCatalogInfo *cat_info = TXFileAllocateFSCtalogInfo(txfile);
	LSItemInfoRecord *iteminfo = TXFileAllocateLSItemInfo(txfile);
	while (!FSGetCatalogInfoBulk(itor, 1, &ict, NULL, whichInfo, cat_info, fsref_p, NULL, &hfs_filename[0])) {
		fname = CFStringCreateWithCharacters(kCFAllocatorDefault, hfs_filename[0].unicode, hfs_filename[0].length);
#if useLog
		fputs("fname in while loop of scanFolder : ", stderr);
		CFShow(fname);
		CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, fsref_p);
		CFShow(url);CFRelease(url);
#endif		
		// resolving alias file
		if (kIsAlias & ((FileInfo *)(cat_info->finderInfo))->finderFlags) {
			Boolean targetIsFolder;
			Boolean wasAliased;
			err = FSResolveAliasFile(fsref_p, true, &targetIsFolder, &wasAliased);
			if (noErr != err) {
				fprintf(stderr, "Failed to FSResolveAliasFile with error : %d\n", err);
				continue;
			}
			
			err = FSGetCatalogInfo(fsref_p, whichInfo, cat_info, NULL, NULL, NULL);
			if (noErr != err) {
				fprintf(stderr, "Failed to FSGetCatalogInfo with error : %d\n", (int)err);
				continue;
			}
		}
		
		Boolean is_package = false;
		Boolean is_folder = false;
		err = LSCopyItemInfoForRef(fsref_p, kLSRequestAllInfo & ~kLSRequestIconAndKind, iteminfo);
		if (noErr != err) {
			fprintf(stderr, "Faild to LSCopyItemInfoForRef with error : %d\n", (int)err);
		}

		if (0 != (cat_info->nodeFlags & kFSNodeIsDirectoryMask)) {
			if (! (iteminfo->flags & kLSItemInfoIsPackage)) {
#if useLog
				UInt8 subfolder_path[PATH_MAX];
				FSRefMakePath(fsref_p, subfolder_path, PATH_MAX);
				fprintf(stderr, "subfolder : %s \n", (char *)subfolder_path);
#endif				
				is_folder = true;
			} else {
				is_package = true;
			}
		}
		
		if (is_folder) {
			if (searchSubFolders) {
				CFURLRef url = CFURLCreateFromFSRef(kCFAllocatorDefault, fsref_p);
				CFArrayAppendValue(subfolders, url);
				CFRelease(url);
			}
		} else {
			module_ref = ModuleRefCreateWithCondition(txfile, fname, module_condition);
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
		safeRelease(iteminfo->extension); iteminfo->extension = NULL;
	}
	
	if (module_ref_candidate) {
		*outRef = module_ref_candidate;
		goto bail;
	}
	
	if (searchSubFolders) {
		for (CFIndex n = 0; n < CFArrayGetCount(subfolders); n++) {
			FSRef subfolder_ref;
			CFURLGetFSRef(CFArrayGetValueAtIndex(subfolders, n), &subfolder_ref);
			err = scanFolder(&subfolder_ref, module_condition, searchSubFolders, outRef);
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
	safeRelease(txfile);
	return err;
}

OSErr findModuleWithName(FSRef *container_ref, ModuleCondition *module_condition, ModuleRef** module_ref)
{
	return scanFolder(container_ref, module_condition, true, module_ref);
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

OSErr pickupModuleAtFolder(FSRef *container_ref, ModuleCondition *module_condition, ModuleRef **out_module_ref)
{
	OSErr err = noErr;
	CFMutableStringRef filename = NULL;
	ModuleRef *module_ref = NULL;
	CFStringRef module_name = module_condition->name;
	FSRef buffer_ref;
	TXFileRef txfile = NULL;
	for (int n = 0; n < NSUFFIXES; n++) {
		filename = CFStringCreateMutableCopy(NULL, 0, module_name);
		CFStringAppendCString(filename, SUFFIXES[n], kCFStringEncodingUTF8);
		err = FSMakeFSRefChild(container_ref, filename, &buffer_ref);
		if (noErr == err) {
			txfile = TXFileCreate(kCFAllocatorDefault, &buffer_ref);
			module_ref = ModuleRefCreate(txfile);
			if (module_ref) {
				if (ModuleConditionVersionIsSatisfied(module_condition, module_ref)) break;
			}
			CFRelease(txfile); txfile = NULL;
			ModuleRefFree(module_ref);module_ref = NULL;
			err = kModuleIsNotFound;
		}
		
		CFRelease(filename);filename = NULL;
	}
	
	if (module_ref) *out_module_ref=module_ref;
bail:
	safeRelease(filename);
	safeRelease(txfile);
	return err;
}

OSErr findModuleWithSubPath(FSRef *container_ref, ModuleCondition *module_condition, ModuleRef** module_ref)
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
			err = scanFolder(&parentdir_ref, module_condition, false, module_ref);
		}
		if (err == noErr) return err;
	}
	return kModuleIsNotFound;
}


OSErr findModule(ModuleCondition *module_condition, CFArrayRef additionalPaths, Boolean ignoreDefaultPaths,
				 ModuleRef** moduleRef, CFMutableArrayRef* searchedPaths)
{
	OSErr err = noErr;
	OSErr (*findModuleAtFolder)(FSRef *container_ref, ModuleCondition *module_condition, ModuleRef** moduleRef);
#if useLog
	fprintf(stderr, "ignoreDefaultPaths : %d\n", ignoreDefaultPaths);
	CFShow(additionalPaths);
#endif	
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
	
	if (!ignoreDefaultPaths) {
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
		if (noErr == noErr) {
			err = findModuleAtFolder(&modules_folder, module_condition, moduleRef);
		} else {
			fprintf(stderr, "Failed to FSPathMakeRef\n");
			CFShow(cf_path);
		}
		free(buffer);
		if (err == noErr) {
#if useLog			
			fprintf(stderr, "Module is found\n");
#endif			
			goto bail;
		}
	}
	
	*searchedPaths = path_list;
	
	if (ignoreDefaultPaths) {
		err = kModuleIsNotFound;
		goto bail;
	}
	
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
