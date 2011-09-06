#define kModuleIsNotFound 1800
#define kFailedToParseVersionCondition 1803

#define BUNDLE_ID CFSTR("Scriptfactory.osax.ModuleLoader")
#define MODULE_PATHS_KEY CFSTR("AdditionalModulePaths")

#define kInDirectoryParam 'inDr'
#define kOtherPathsParam 'ohPh'
#define kForModuleParam 'forM'

#define kModuleLoaderSuite  'Molo'
#define kLoadModuleEvent	'loMo'
#define kPrivateLoadModuleEvent 'PloM'
#define kFindModuleEvent	'fdMo'
#define kMakeLoaderEvent	'MKlo'
#define kPrivateMakeLoaderEvent	'PMKl'
#define kMakeLocaLoaderEvent 'MkLl'
#define kSetAdditionalModulePathsEvent 'adMp'
#define kModulePathsEvent 'gtPH'
#define kVersionEvent 'Vers'
#define kMakeModuleSpecEvent 'MkMs'
#define kExtractDependenciesEvent 'Exdp'
#define kReloadingParam 'pRLo'
#define kVersionParam 'vers'
#define kConditionParam 'ConD'

enum {
	typeModuleSpecifier = 'MoSp',
};
