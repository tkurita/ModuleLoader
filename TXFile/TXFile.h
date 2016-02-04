typedef CFDataRef TXFileRef;

TXFileRef TXFileCreate(CFAllocatorRef allocator, FSRef *fsref);
LSItemInfoRecord *TXFileAllocateLSItemInfo(TXFileRef txfile);
LSItemInfoRecord *TXFileGetLSItemInfo(TXFileRef txfile, Boolean refresh, OSErr *err);
FSCatalogInfo *TXFileAllocateFSCtalogInfo(TXFileRef txfile);
FSCatalogInfo *TXFileGetFSCatalogInfo(TXFileRef txfile, FSCatalogInfoBitmap whichinfo, Boolean refresh, OSErr *err);
OSErr TXFileResolveAlias(TXFileRef txfile, Boolean *wasAliased);
Boolean TXFileIsDirectory(TXFileRef txfile, OSErr *err);
Boolean TXFileIsPackage(TXFileRef txfile, OSErr *err);
FSRef *TXFileGetFSRefPtr(TXFileRef txfile);
