#include <CoreFoundation/CoreFoundation.h>
#include "TXFile.h"

#define useLog 0

#define SafeRelease(v) if(v) CFRelease(v)
#define TXFileGetStruct(x) (TXFileStruct *)CFDataGetBytePtr(x)

typedef struct  {
	FSRef fsref;
	LSItemInfoRecord *lsInfo;
	FSCatalogInfo *catInfo;
    CFURLRef urlref;
} TXFileStruct;

static void TXFileDeallocate(void *ptr, void *info)
{
#if useLog
	fputs("TXFileDeallocate\n", stderr);
#endif	
	TXFileStruct *txf_struct = (TXFileStruct *)ptr;
	if (txf_struct->lsInfo) {
		SafeRelease(txf_struct->lsInfo->extension);
		free(txf_struct->lsInfo);
	}
	
	free(txf_struct->catInfo);
	free(txf_struct);
    CFRelease(txf_struct->urlref);
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

TXFileRef TXFileCreate(CFAllocatorRef allocator, FSRef *fsref)
{
#if useLog
	fputs("TXFileCreate\n", stderr);
#endif		
	TXFileStruct *txf_struct = malloc(sizeof(TXFileStruct));
	if (!txf_struct) return NULL;
    if (fsref) {
        txf_struct->fsref = *fsref;
        txf_struct->urlref = CFURLCreateFromFSRef(kCFAllocatorDefault, fsref);
    }
	txf_struct->lsInfo = NULL;
	txf_struct->catInfo = NULL;
	
	
	CFAllocatorRef deallocator = CreateTXFileDeallocator();
	TXFileRef txfile = CFDataCreateWithBytesNoCopy(allocator, (const UInt8 *)txf_struct, 
													sizeof(TXFileStruct), deallocator);
	return txfile;
}

CFURLRef CFURLGetFromTXFile(TXFileRef txfile)
{
    TXFileStruct *txf_struct = TXFileGetStruct(txfile);
    return txf_struct->urlref;
}

FSRef *TXFileGetFSRefPtr(TXFileRef txfile)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
	return &(txf_struct->fsref);
}

LSItemInfoRecord *TXFileAllocateLSItemInfo(TXFileRef txfile)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
	if (! txf_struct->lsInfo) {
		txf_struct->lsInfo = (LSItemInfoRecord *)malloc(sizeof(LSItemInfoRecord));
		txf_struct->lsInfo->extension = NULL;
	}
	return txf_struct->lsInfo;
}

LSItemInfoRecord *TXFileGetLSItemInfo(TXFileRef txfile, Boolean refresh, OSErr *err)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
	if (txf_struct->lsInfo) {
		if (refresh) {
			CFRelease(txf_struct->lsInfo->extension);
		} else {
			goto bail;
		}
	} else {
		txf_struct->lsInfo = (LSItemInfoRecord *)malloc(sizeof(LSItemInfoRecord));
		txf_struct->lsInfo->extension = NULL;
	}
	*err = LSCopyItemInfoForRef(&(txf_struct->fsref), kLSRequestAllInfo & ~kLSRequestIconAndKind, txf_struct->lsInfo);
bail:	
	return txf_struct->lsInfo;
}

FSCatalogInfo *TXFileAllocateFSCtalogInfo(TXFileRef txfile)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
	if (! txf_struct->catInfo) {
		txf_struct->catInfo = (FSCatalogInfo *)malloc(sizeof(FSCatalogInfo));
	}
	return txf_struct->catInfo;	
}

FSCatalogInfo *TXFileGetFSCatalogInfo(TXFileRef txfile, FSCatalogInfoBitmap whichinfo, Boolean refresh, OSErr *err)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
	if (txf_struct->catInfo) {
		if (! refresh) goto bail;
	} else {
		txf_struct->catInfo = (FSCatalogInfo *)malloc(sizeof(FSCatalogInfo));	
	}
	
	*err = FSGetCatalogInfo(&(txf_struct->fsref), whichinfo, txf_struct->catInfo, NULL, NULL, NULL);
	if (noErr != *err) {
		return NULL;
	}
	
bail:	
	return txf_struct->catInfo;
}

void TXFileReleaseInfo(TXFileRef txfile)
{
	TXFileStruct *txf_struct = TXFileGetStruct(txfile);
	free(txf_struct->catInfo);
	txf_struct->catInfo = NULL;
	if (txf_struct->lsInfo) {
		SafeRelease(txf_struct->lsInfo->extension);
		free(txf_struct->lsInfo);
		txf_struct->lsInfo = NULL;
	}
}

OSErr TXFileResolveAlias(TXFileRef txfile, Boolean *wasAliased)
{
	OSErr err = noErr;
	FSCatalogInfo *cat_info = TXFileGetFSCatalogInfo(txfile, kFSCatInfoFinderInfo|kFSCatInfoNodeFlags,
													 false, &err);
	if (noErr != err) goto bail;
	
	if (kIsAlias & ((FileInfo *)(&cat_info->finderInfo))->finderFlags) { // resolve alias
		Boolean targetIsFolder;
		err = FSResolveAliasFile(TXFileGetFSRefPtr(txfile), true, &targetIsFolder, wasAliased);
		if (noErr != err) {
			fprintf(stderr, "Failed to FSResolveAliasFile with error : %d\n", (int)err);
			goto bail;
		}
		TXFileReleaseInfo(txfile);
	}
	
bail:
	return err;
}

Boolean TXFileIsDirectory(TXFileRef txfile, OSErr *err)
{
	Boolean result = false;
	FSCatalogInfo *cat_info = TXFileGetFSCatalogInfo(txfile,  kFSCatInfoFinderInfo|kFSCatInfoNodeFlags,
													 0, err);
	if (noErr != *err) {
		fprintf(stderr, "Failed TXFileGetFSCatalogInfo with	error : %d\n", (int)*err);
		goto bail;
	}
	
	result = cat_info->nodeFlags & kFSNodeIsDirectoryMask;
bail:
	return result;
}

Boolean TXFileIsPackage(TXFileRef txfile, OSErr *err)
{
	Boolean result = false;
	LSItemInfoRecord *iteminfo = TXFileGetLSItemInfo(txfile, 0, err);
	if (noErr != *err) {
		goto bail;
	}
	result = (iteminfo->flags & kLSItemInfoIsPackage);
bail:
	return result;	
}