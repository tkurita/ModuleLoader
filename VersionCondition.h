#pragma mark VersionCondition

typedef struct  {
	CFStringRef version_string;
	CFComparisonResult less_or_greater;
	Boolean allow_equal;
} VersionCondition;

VersionCondition *VersionConditionCreate(CFStringRef opstring, CFStringRef verstring);
VersionCondition *VersionConditionCreateWithString(CFStringRef condition, CFStringRef *errmsg);
void VersionConditionFree(VersionCondition *vc);
Boolean VersionConditionIsSatisfied(VersionCondition *condition, CFStringRef version);

#pragma mark VersionConditionSet
typedef struct {
	VersionCondition **conditions;
	unsigned long length;
} VersionConditionSet;
                     
VersionConditionSet *VersionConditionSetCreate(CFStringRef condition, CFStringRef *errmsg);
void VersionConditionSetFree(VersionConditionSet *vc);
Boolean VersionConditionSetIsSatisfied(VersionConditionSet *vercond_set, CFStringRef version);

