#include "ModuleCache.h"

static CFMutableDictionaryRef CACHE = NULL;

void initializeCache()
{
	if (CACHE == NULL) {
		CACHE = CFDictionaryCreateMutable (NULL, 5, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
	}
}

OSAID findModuleInCache(CFStringRef name)
{
	
	CFNumberRef result = CFDictionaryGetValue(CACHE, name);
	if (result == NULL) {
		return kOSANullScript;
	}
	OSAID module_id;
	CFNumberGetValue(result, kCFNumberCFIndexType, &module_id);
	return module_id;
}

void storeModuleInCache(CFStringRef name, OSAID module)
{
	CFNumberRef boxedmod = CFNumberCreate(NULL, kCFNumberCFIndexType, &module);
	CFDictionarySetValue(CACHE, name, boxedmod);
	CFRelease(boxedmod);
}
