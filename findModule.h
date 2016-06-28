#include "ModuleCondition.h"

void setAdditionalModulePaths(CFArrayRef array);
CFArrayRef additionalModulePaths();
CFArrayRef copyDefaultModulePaths();
OSErr findModuleWithName(FSRef *container_ref, ModuleCondition *module_condition, ModuleRef** moudle_ref);
OSErr findModuleWithSubPath(FSRef *container_ref, ModuleCondition *module_condition, ModuleRef** module_ref);
OSErr FSMakeFSRefChild(FSRef *parentRef, CFStringRef childName, FSRef *newRef);
OSErr findModule(ModuleCondition *module_condition, CFArrayRef additionalPaths, Boolean ignoreDefaultPaths,
				 ModuleRef** moduleRef, CFMutableArrayRef* searchedPaths);
OSErr pickupModuleAtFolder(CFURLRef container_url, ModuleCondition* module_condition, ModuleRef** out_module_ref);

