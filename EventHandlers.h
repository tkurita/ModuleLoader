#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_5
typedef SInt32                          SRefCon;
#endif

OSErr versionHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr makeLocalLoaderHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr modulePathsHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr setAdditionalModulePathsHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr loadModuleHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr findModuleHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr makeLoaderHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr makeModuleSpecHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr extractDependenciesHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr _loadModuleHandler_(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);
OSErr meetVersionHandler(const AppleEvent *ev, AppleEvent *reply, SRefCon refcon);