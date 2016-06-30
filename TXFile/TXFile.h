typedef CFDataRef TXFileRef;

TXFileRef TXFileCreate(CFAllocatorRef allocator, FSRef *fsref);
TXFileRef TXFileCreateWithURL(CFAllocatorRef allocator, CFURLRef url);
Boolean TXFileResolveAlias(TXFileRef txfile, CFErrorRef *error);
Boolean TXFileTest(TXFileRef txfile, CFStringRef key, CFErrorRef *error);
Boolean TXFileIsAliasFile(TXFileRef txfile, CFErrorRef *error);
Boolean TXFileIsDirectory(TXFileRef txfile, CFErrorRef *error);
Boolean TXFileIsPackage(TXFileRef txfile, CFErrorRef *error);
CFURLRef TXFileGetURL(TXFileRef txfile);
CFURLRef TXFileCopyURL(TXFileRef txfile);
CFTypeRef TXFileCopyAttribute(TXFileRef txfile, CFStringRef key, CFErrorRef *error);
CFStringRef TXFileCopyTypeIdentifier(TXFileRef txfile, CFErrorRef *error);