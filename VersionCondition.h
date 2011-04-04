#pragma mark VersionCondition

typedef struct  {
	CFStringRef version_string;
	CFComparisonResult less_or_greater;
	Boolean allow_equal;
} VersionCondition;

VersionCondition *VersionCoditionCreate(CFStringRef condition);
void VersionConditionFree(VersionCondition *vc);
Boolean VersionConditionSatisfied(VersionCondition *condition, CFStringRef version);

#pragma mark VersionConditionSet
typedef struct {
	VersionCondition **conditions;
	unsigned long length;
} VersionConditionSet;

VersionConditionSet *VersionCoditionSetCreate(CFStringRef condition);
void VersionConditionSetFree(VersionCondition *vc);
Boolean VersionConditionSetSatisfied(VersionConditionSet *vercond_set, CFStringRef version);

