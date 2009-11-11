#include <Carbon/Carbon.h>

void initializeCache();
OSAID findModuleInCache(CFStringRef name);
void storeModuleInCache(CFStringRef name, OSAID module);
