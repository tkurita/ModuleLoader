@interface BAObjectProto : NSObject
{
}

+ (void)forwardInvocation:(id)arg1;
+ (BOOL)hasScriptHandler:(id)arg1;
+ (id)invokeScriptHandler:(id)arg1 args:(id)arg2 error:(id *)arg3;
+ (id)invokeScriptHandler:(id)arg1 forObject:(unsigned int)arg2 inComponentInstance:(struct ComponentInstanceRecord *)arg3 args:(id)arg4 error:(id *)arg5;
+ (unsigned int)getOSAID;
+ (struct ComponentInstanceRecord *)getComponentInstance;
- (id)description;
- (void)dumpProperties:(id)arg1;
- (void)forwardInvocation:(id)arg1;
- (id)methodSignatureForSelector:(SEL)arg1;
- (void)doesNotRecognizeSelector:(SEL)arg1;
- (BOOL)respondsToSelector:(SEL)arg1;
- (BOOL)scriptRespondsToSelector:(SEL)arg1;
- (BOOL)scriptHasSetter:(id)arg1;
- (BOOL)scriptHasGetter:(id)arg1;
- (void)setValue:(id)arg1 forUndefinedKey:(id)arg2;
- (id)valueForUndefinedKey:(id)arg1;
- (BOOL)hasScriptProperty:(id)arg1;
- (BOOL)hasScriptHandler:(id)arg1;
- (id)invokeScriptHandler:(id)arg1 args:(id)arg2 error:(id *)arg3;
- (id)handlerNames;
- (id)setterNames;
- (id)_allProperties;
- (id)propertyNames;
- (void)finalize;
- (void)dealloc;
- (void)setOSAID:(unsigned int)arg1;
- (unsigned int)getOSAID;
- (unsigned int)_getOSAID;
- (void)setComponentInstance:(struct ComponentInstanceRecord *)arg1;
- (struct ComponentInstanceRecord *)getComponentInstance;
- (struct ComponentInstanceRecord *)_getComponentInstance;

@end

@interface BAGenericObject : BAObjectProto
{
    struct ComponentInstanceRecord *m_ci;
    unsigned int m_oid;
}

@end

@interface BAGenericObjectNoDeleteOSAID : BAGenericObject
{
}

- (void)dealloc;

@end
