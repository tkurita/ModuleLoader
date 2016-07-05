#ifndef ModuleCondition_h
#define ModuleCondition_h

#include "VersionCondition.h"
#include "TXRegularExpression.h"

typedef struct {
	CFStringRef name;
	CFArrayRef subpath;
	VersionConditionSet *required_version;
	TXRegexRef pattern;
} ModuleCondition;

#include "ModuleRef.h"

ModuleCondition *ModuleConditionCreate(CFStringRef name, CFStringRef version, Boolean hfs_style, CFStringRef *errmsg);
void ModuleConditionFree(ModuleCondition *module_condition);
Boolean ModuleConditionVersionIsSatisfied(ModuleCondition *module_condition, ModuleRef *module_ref);
Boolean ModuleConditionHasSubpath(ModuleCondition *module_condition);
CFArrayRef ModuleConditionParseName(ModuleCondition *module_condition, CFStringRef name);

#endif