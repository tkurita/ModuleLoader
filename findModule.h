#ifndef findModule_h
#define findModule_h

#include "ModuleCondition.h"

void setAdditionalModulePaths(CFArrayRef array);
NSArray *additionalModulePaths();
NSArray *copyDefaultModulePaths();
OSErr findModuleWithName(NSURL *container_url, ModuleCondition *module_condition, ModuleRef** moudle_ref);
OSErr findModuleWithSubPath(NSURL *container_url, ModuleCondition *module_condition, ModuleRef** module_ref);
OSErr findModule(ModuleCondition *module_condition, CFArrayRef additionalPaths, Boolean ignoreDefaultPaths,
				 ModuleRef** moduleRef, CFMutableArrayRef* searchedPaths);
OSErr pickupModuleAtFolder(CFURLRef container_url, ModuleCondition* module_condition, ModuleRef** out_module_ref);


#endif