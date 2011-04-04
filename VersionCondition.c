#include <CoreFoundation/CoreFoundation.h>
#include "VersionCondition.h"
#include "TXRegularExpression/TXRegularExpression.h"

#pragma mark VersoionCondition

TXRegularExpression *VersionConditionPattern()
{
	static TXRegularExpression *verpattern = NULL;
	if (!verpattern) {
		UParseError pe;
		UErrorCode status = U_ZERO_ERROR;
		verpattern = TXRegexCreate(CFSTR("\\s*(<|>|>=|=<)?\\s*([0-9\\.]+)\\s*"), 0, &pe, &status);
		if (U_ZERO_ERROR != status) {
			fprintf(stderr, "Failed to compile pattern of version with error : %d\n", status);
			fprintParseError(stderr, &pe);
			return NULL;
		}
	}
	return verpattern;
}

VersionCondition *VersionConditionCreate(CFStringRef opstring, CFStringRef verstring)
{
	VersionCondition *vc = malloc(sizeof(VersionCondition));
	if (CFStringCompare(opstring, CFSTR(""), 0) == kCFCompareEqualTo) {
		vc->less_or_greater = kCFCompareGreaterThan;
		vc->allow_equal = true;
		goto bail;
	}	
	if (CFStringCompare(opstring, CFSTR(">"), 0) == kCFCompareEqualTo) {
		vc->less_or_greater = kCFCompareGreaterThan;
		vc->allow_equal = false;
		goto bail;
	}
	if (CFStringCompare(opstring, CFSTR("<"), 0) == kCFCompareEqualTo) {
		vc->less_or_greater = kCFCompareLessThan;
		vc->allow_equal = false;
		goto bail;
	}
	if (CFStringCompare(opstring, CFSTR(">="), 0) == kCFCompareEqualTo) {
		vc->less_or_greater = kCFCompareGreaterThan;
		vc->allow_equal = true;
		goto bail;
	}
	if (CFStringCompare(opstring, CFSTR("<="), 0) == kCFCompareEqualTo) {
		vc->less_or_greater = kCFCompareLessThan;
		vc->allow_equal = true;
		goto bail;
	}
	free(vc);
	return NULL;
bail:
	CFRetain(verstring);
	vc->version_string = verstring;
	return vc;
}

VersionCondition *VersionCoditionCreateWithString(CFStringRef condition)
{
	TXRegularExpression *verpattern = VersionConditionPattern();
	if (!verpattern) return NULL;
	UErrorCode status = U_ZERO_ERROR;
	CFArrayRef matched = CFStringCreateArrayWithFirstMatch(condition,  verpattern, 0, &status);
	if (!matched) return NULL;
	
	VersionCondition *vc = VersionConditionCreate(CFArrayGetValueAtIndex(matched, 1), CFArrayGetValueAtIndex(matched, 2));	
	CFRelease(matched);
    return vc;
}

void VersionConditionFree(VersionCondition *vc)
{
	CFRelease(vc->version_string);
	free(vc);
}

Boolean VersionConditionSatisfied(VersionCondition *condition, CFStringRef version)
{
	CFComparisonResult compresult = CFStringCompare(version, condition->version_string, kCFCompareNumerically);
	Boolean is_satisfy = false;
	switch (compresult) {
		case kCFCompareEqualTo:
			if (condition->allow_equal) is_satisfy =true;			
			break;
		default:
			if (compresult == condition->less_or_greater) is_satisfy = true;
			break;
	}
	
	return is_satisfy;
}

#pragma mark VersionConditionSet

VersionConditionSet *VersionCoditionSetCreate(CFStringRef condition)
{
	CFArrayRef array = NULL;
	VersionConditionSet *vercond_set = NULL;
	VersionCondition **vercond_list = NULL;
	
	TXRegularExpression *verpattern = VersionConditionPattern();
	if (!verpattern) return NULL;
	UErrorCode status = U_ZERO_ERROR;
	array = CFStringCreateArrayWithAllMatches(condition, verpattern, &status);
	if (U_ZERO_ERROR != status) {
		fprintf(stderr, "Error in VersionCoditionSetCreate number : %d\n", status);
		goto bail;
	}
	CFIndex len = CFArrayGetCount(array);
	if (!len) {
		fprintf(stderr, "No matches\n");
		goto bail;
	}
		
	vercond_list = (VersionCondition **)malloc(len*sizeof(VersionCondition *));
	if (!vercond_list) {
		fprintf(stderr, "Failed to allocate an array of VersionCondition");
		goto bail;
	}
	
	for (CFIndex n=0; n < len; n++) {
		CFArrayRef subarray = CFArrayGetValueAtIndex(array, n);
		VersionCondition *vercond = VersionConditionCreate(CFArrayGetValueAtIndex(subarray, 1),
														   CFArrayGetValueAtIndex(subarray, 2));
		vercond_list[n] = vercond;
	}
	vercond_set = malloc(sizeof(VersionConditionSet));
	if (!vercond_set) {
		fprintf(stderr, "Failed to allocate VersionConditionSet\n");
		for (CFIndex n=0; n < len; n++) {
			free(vercond_list[n]);
		}
		free(vercond_list);
		goto bail;
	}
	vercond_set->length = len;
	vercond_set->conditions = vercond_list;
	
bail:	
	if(!array) CFRelease(array);	
	return vercond_set;
}

void VersionCoditionSetFree(VersionConditionSet *vercond_set)
{
	VersionCondition **vclist = vercond_set->conditions;
	for (CFIndex n = 0; n < vercond_set->length; n++) {
		VersionConditionFree(vclist[n]);
	}
	free(vclist);
}

Boolean VersionConditionSetSatisfied(VersionConditionSet *vercond_set, CFStringRef version)
{
	VersionCondition **vclist = vercond_set->conditions;
	for (CFIndex n = 0; n < vercond_set->length; n++) {
		if (!VersionConditionSatisfied(vclist[n], version)) return false;
	}
	return true;
}