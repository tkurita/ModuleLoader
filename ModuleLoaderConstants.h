#define STR(s) _STR(s)
#define _STR(s) #s

#define BUNDLE_ID CFSTR(STR(PRODUCT_BUNDLE_IDENTIFIER))
#define MODULE_PATHS_KEY CFSTR("AdditionalModulePaths")

#define kModuleLoaderSuite  'Molo'

enum ModuleLoaderCommands {
    kLoadModuleEvent = 'loMo',
    kFindModuleEvent = 'fdMo',
    kPrivateLoadModuleEvent = 'PloM',
    kMakeLoaderEvent = 'MKlo',
    kPrivateMakeLoaderEvent	= 'PMKl',
    kMakeLocaLoaderEvent = 'MkLl',
    kExtractDependenciesEvent = 'Exdp',
    kSetAdditionalModulePathsEvent = 'adMp',
    kModulePathsEvent = 'gtPH',
    kMakeModuleSpecEvent = 'MkMs',
    kVersionEvent = 'Vers'
};

enum HandlerLabels {
    kInDirectoryParam = 'inDr',
    kOtherPathsParam = 'ohPh',
    kConditionParam = 'ConD'
};

enum ModuleSpecifierProperties {
    kReloadingParam = 'pRLo' ,
    kVersionParam = 'vers' ,
    kFromUseParam = 'fmUs'
};

enum {
	typeModuleSpecifier = 'MoSp',
};

enum ModuleLoaderErros {
    kModuleIsNotFound = 1800,
    kModuleLoaderInternalError = 1802,
    kFailedToParseVersionCondition = 1803
};