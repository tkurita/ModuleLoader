#include "TXFile.h"

typedef struct {
	FSRef fsref;
	CFStringRef name;
	CFStringRef version;
	Boolean is_package;
    CFURLRef url;
} ModuleRef;

CFStringRef ModuleRefGetVersion(ModuleRef *module_ref);
ModuleRef *ModuleRefCreate(TXFileRef txfile);
ModuleRef *ModuleRefCreateWithCondition(TXFileRef txfile, ModuleCondition *module_condition);
void ModuleRefFree(ModuleRef *module_ref);
CFComparisonResult ModuleRefCompareVersion(ModuleRef *module_ref1, ModuleRef *module_ref2);
void ShowModuleRef(ModuleRef *module_ref);