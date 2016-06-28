#include <CoreFoundation/CoreFoundation.h>
#include "TXFile.h"

#define useLog 0

#define SafeRelease(v) if(v) CFRelease(v)
#define TXFileGetStruct(x) (TXFileStruct *)CFDataGetBytePtr(x)

typedef struct  {
	FSRef fsref;
    CFURLRef url;
} TXFileStruct;

static void TXFileDeallocate(void *ptr, void *info)
{
#if useLog
	fputs("TXFileDeallocate\n", stderr);
#endif	
	TXFileStruct *txf_struct = (TXFileStruct *)ptr;
    
    if (txf_struct->url) CFRelease(txf_struct->url);
    free(txf_struct);
}

static CFAllocatorRef CreateTXFileDeallocator(void) {
    static CFAllocatorRef allocator = NULL;
    if (!allocator) {
        CFAllocatorContext context =
		{0, // version
			NULL, //info
			NULL, // retain callback
			(void *)free,  //  CFAllocatorReleaseCallBack
			NULL, // CFAllocatorCopyDescriptionCallBack
			NULL, //CFAllocatorAllocateCallBack
			NULL, // CFAllocatorReallocateCallBack 
			TXFileDeallocate, //CFAllocatorDeallocateCallBack 
			NULL //CFAllocatorPreferredSizeCallBack 
		};
        allocator = CFAllocatorCreate(NULL, &context);
    }
    return allocator;
}

TXFileRef TXFileCreateWithURL(CFAllocatorRef allocator, CFURLRef url)
{
#if useLog
    fputs("TXFileCreateWithURL\n", stderr);
#endif
    TXFileStruct *txf_struct = malloc(sizeof(TXFileStruct));
    if (!txf_struct) return NULL;
    if (url) {
        CFRetain(url);
        txf_struct->url = url;
        if (! CFURLGetFSRef(url, &(txf_struct->fsref))) {
            fprintf(stderr, "Faild to get FSRef from CFURL\n");
        }
        
    }
    
    CFAllocatorRef deallocator = CreateTXFileDeallocator();
    TXFileRef txfile = CFDataCreateWithBytesNoCopy(allocator, (const UInt8 *)txf_struct,
                                                   sizeof(TXFileStruct), deallocator);
    return txfile;
}

TXFileRef TXFileCreate(CFAllocatorRef allocator, FSRef *fsref)
{
#if useLog
	fputs("TXFileCreate\n", stderr);
#endif		
	TXFileStruct *txf_struct = malloc(sizeof(TXFileStruct));
	if (!txf_struct) return NULL;
    if (fsref) {
        txf_struct->fsref = *fsref;
        txf_struct->url = CFURLCreateFromFSRef(kCFAllocatorDefault, fsref);
    }
	
	CFAllocatorRef deallocator = CreateTXFileDeallocator();
	TXFileRef txfile = CFDataCreateWithBytesNoCopy(allocator, (const UInt8 *)txf_struct, 
													sizeof(TXFileStruct), deallocator);
	return txfile;
}

CFURLRef TXFileCopyURL(TXFileRef txfile)
{
    TXFileStruct *txf_struct = TXFileGetStruct(txfile);
    CFRetain(txf_struct->url);
    return txf_struct->url;
}

CFURLRef TXFileGetURL(TXFileRef txfile)
{
    TXFileStruct *txf_struct = TXFileGetStruct(txfile);
    return txf_struct->url;
}

FSRef *TXFileGetFSRefPtr(TXFileRef txfile)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
	return &(txf_struct->fsref);
}

void TXFileReleaseInfo(TXFileRef txfile)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
}

Boolean TXFileResolveAlias(TXFileRef txfile, CFErrorRef *error)
{
    if (! TXFileIsAliasFile(txfile, error)) {
        return true;
    }
    Boolean result = false;
    TXFileStruct *txf_struct = TXFileGetStruct(txfile);
    CFDataRef bookmark_data = NULL;
    
    
    bookmark_data = CFURLCreateBookmarkDataFromFile(kCFAllocatorDefault,
                                                    txf_struct->url,
                                                    error);
    if (error) {
        goto bail;
    }
    Boolean is_stale;
    CFURLRef new_url = CFURLCreateByResolvingBookmarkData(kCFAllocatorDefault,
                                                          bookmark_data,
                                                          kCFBookmarkResolutionWithoutUIMask,
                                                          NULL,
                                                          NULL,
                                                          &is_stale,
                                                          error);
    if (error) {
        SafeRelease(new_url);
        goto bail;
    }
    
    if (is_stale) {
        SafeRelease(new_url);
        CFShow(txf_struct->url);
        fprintf(stderr, "The Alias file is slate.\n");
        goto bail;
    }
    
    result = true;
    CFRelease(txf_struct->url);
    txf_struct->url = new_url;	
bail:
    SafeRelease(bookmark_data);
	return result;
}

Boolean TXFileTest(TXFileRef txfile, CFStringRef key, CFErrorRef *error)
{
    TXFileStruct *txf_struct = TXFileGetStruct(txfile);
    CFBooleanRef is_key = NULL;
    Boolean result = false;
    if (CFURLCopyResourcePropertyForKey(txf_struct->url,
                                        key,
                                        &is_key,
                                        error)) {
        if (is_key) {
            result = CFBooleanGetValue(is_key);
        }
    }
    SafeRelease(is_key);
    return result;
}

Boolean TXFileIsAliasFile(TXFileRef txfile, CFErrorRef *error)
{
    return TXFileTest(txfile, kCFURLIsAliasFileKey, error);
}

Boolean TXFileIsDirectory(TXFileRef txfile, CFErrorRef *error)
{
    return TXFileTest(txfile, kCFURLIsDirectoryKey, error);
}

Boolean TXFileIsPackage(TXFileRef txfile, CFErrorRef *error)
{
    return TXFileTest(txfile, kCFURLIsPackageKey, error);
}

CFTypeRef TXFileCopyAttribute(TXFileRef txfile, CFStringRef key, CFErrorRef *error)
{
    TXFileStruct *txf_struct = TXFileGetStruct(txfile);
    CFTypeRef attr = NULL;
    if (CFURLCopyResourcePropertyForKey(txf_struct->url,
                                        key,
                                        &attr,
                                        error)) {
        return attr;
    }
    return NULL;
}

CFStringRef TXFileCopyTypeIdentifier(TXFileRef txfile, CFErrorRef *error)
{
    return TXFileCopyAttribute(txfile, kCFURLTypeIdentifierKey, error);
}