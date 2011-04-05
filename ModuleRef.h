typedef struct {
	FSRef fsref;
	CFStringRef name;
	CFStringRef version;
	Boolean is_package;
} ModuleRef;

CFStringRef ModuleRefGetVersion(ModuleRef *module_ref);
ModuleRef *ModuleRefCreate(FSRef *fsref, FSCatalogInfo *cat_info);
ModuleRef *ModuleRefCreateWithCondition(FSRef *fsref, FSCatalogInfo *cat_info, CFStringRef filename,
										Boolean is_package, ModuleCondition *module_condition);
void ModuleRefFree(ModuleRef *module_ref);
CFComparisonResult ModuleRefCompareVersion(ModuleRef *module_ref1, ModuleRef *module_ref2);