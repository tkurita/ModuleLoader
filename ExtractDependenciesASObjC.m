#include <Carbon/Carbon.h>
#import <Foundation/Foundation.h>
#import "BAGenericObject.h"
#include "ExtractDependencies.h"

#define useLog 0

OSErr extractDependenciesASObjC(AEDesc *script_data_ptr, AEDesc *dependencies_ptr)
{
    OSErr err;
    Size datasize;
    AEDesc ocid = {typeNull, NULL};
    id ocid_value;
    OSAID script_id = kOSANullScript;
    ComponentInstance scriptingComponent = NULL;
    err = AEGetKeyDesc(script_data_ptr, keyAEKeyData, 'optr', &ocid);
    datasize = AEGetDescDataSize(&ocid);
    AEGetDescData(&ocid, &ocid_value, datasize);
#if useLog
    NSLog(@"ocid_value : %@", ocid_value);
#endif
    script_id = [ocid_value getOSAID];
    scriptingComponent = [ocid_value getComponentInstance];
    
    err = extractDependencies(scriptingComponent, script_id, dependencies_ptr);
bail:
    AEDisposeDesc(&ocid);
    return err;
}
