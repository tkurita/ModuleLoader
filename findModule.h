OSErr findModuleWithName(FSRef *container_ref, CFTypeRef module_name, FSRef* moudle_ref);
OSErr findModuleWithSubPath(FSRef *container_ref, CFTypeRef path_components, FSRef* module_ref);
OSErr FSMakeFSRefChild(FSRef *parentRef, CFStringRef childName, FSRef *newRef);
OSErr findModule(CFStringRef moduleName, FSRef *moduleRef);