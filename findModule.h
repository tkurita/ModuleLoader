void setAdditionalModulePaths(CFArrayRef array);
CFArrayRef additionalModulePaths();
CFArrayRef copyDefaultModulePaths();
OSErr findModuleWithName(FSRef *container_ref, CFTypeRef module_name, FSRef* moudle_ref);
OSErr findModuleWithSubPath(FSRef *container_ref, CFTypeRef path_components, FSRef* module_ref);
OSErr FSMakeFSRefChild(FSRef *parentRef, CFStringRef childName, FSRef *newRef);
OSErr findModule(CFStringRef moduleName, CFArrayRef additionalPaths, Boolean ingoreDefaultPaths,
				 FSRef *moduleRef, CFMutableArrayRef* searcedPaths);

#define kModuleIsNotFound 1800
#define BUNDLE_ID CFSTR("Scriptfactory.ModuleLoaderOSAX")