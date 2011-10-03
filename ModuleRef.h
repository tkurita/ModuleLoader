typedef struct {
	FSRef fsref;
	CFStringRef name;
	CFStringRef version;
	Boolean is_package;
} ModuleRef;

CFStringRef ModuleRefGetVersion(ModuleRef *module_ref);
ModuleRef *ModuleRefCreate(FSRef *fsref, FSCatalogInfo *cat_info);
ModuleRef *ModuleRefCreateWithCondition(FSRef *fsref, FSCatalogInfo *cat_info, CFStringRef filename, 
										LSItemInfoRecord* iteminfo_p, ModuleCondition *module_condition);

void ModuleRefFree(ModuleRef *module_ref);
CFComparisonResult ModuleRefCompareVersion(ModuleRef *module_ref1, ModuleRef *module_ref2);
void ShowModuleRef(ModuleRef *module_ref);