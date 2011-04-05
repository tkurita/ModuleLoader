#include "ModuleCondition.h"

void setAdditionalModulePaths(CFArrayRef array);
CFArrayRef additionalModulePaths();
CFArrayRef copyDefaultModulePaths();
OSErr findModuleWithName(FSRef *container_ref, ModuleCondition *module_condition, FSRef* moudle_ref);
OSErr findModuleWithSubPath(FSRef *container_ref, ModuleCondition *module_condition, FSRef* module_ref);
OSErr FSMakeFSRefChild(FSRef *parentRef, CFStringRef childName, FSRef *newRef);
OSErr findModule(ModuleCondition *module_condition, CFArrayRef additionalPaths, Boolean ingoreDefaultPaths,
				 FSRef *moduleRef, CFMutableArrayRef* searchedPaths);
OSErr pickupModuleAtFolder(FSRef *container_ref, ModuleCondition *module_condition, FSRef* out_module_ref);

