#include <CoreFoundation/CoreFoundation.h>
#include "TXRegularExpression/TXRegularExpression.h"
#include "VersionCondition.h"

#include "AEUtils.h"

#pragma mark VersoionCondition

TXRegularExpression *VersionConditionPattern(CFStringRef *errmsg)
{
	static TXRegularExpression *verpattern = NULL;
	if (!verpattern) {
		UParseError pe;
		UErrorCode status = U_ZERO_ERROR;
		verpattern = TXRegexCreate(CFSTR("\\s*(<|>|>=|=<)?\\s*([0-9\\.]+)\\s*"), 0, &pe, &status);
		if (U_ZERO_ERROR != status) {
			CFStringRef pemsg = CFStringCreateWithFormattingParseError(&pe);
			*errmsg = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
												 CFSTR("Failed to compile pattern of VersionCondition. %@"),
												 pemsg);
			safeRelease(pemsg);
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
	vc->version_string = CFRetain(verstring);
	return vc;
}

VersionCondition *VersionConditionCreateWithString(CFStringRef condition, CFStringRef *errmsg)
{
	TXRegularExpression *verpattern = VersionConditionPattern(errmsg);
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

Boolean VersionConditionIsSatisfied(VersionCondition *condition, CFStringRef version)
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

VersionConditionSet *VersionConditionSetCreate(CFStringRef condition, CFStringRef *errmsg)
{
	CFArrayRef array = NULL;
	VersionConditionSet *vercond_set = NULL;
	VersionCondition **vercond_list = NULL;
	
	TXRegularExpression *verpattern = VersionConditionPattern(errmsg);
	if (!verpattern) return NULL;
	UErrorCode status = U_ZERO_ERROR;
	array = CFStringCreateArrayWithAllMatches(condition, verpattern, &status);
	if (U_ZERO_ERROR != status) {
		*errmsg = CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
							   CFSTR("Error in VersionCoditionSetCreate number : %d"), status);
		goto bail;
	}
	CFIndex len = CFArrayGetCount(array);
	if (!len) {
		*errmsg = CFSTR("No mathes in Version condition string.");
		goto bail;
	}
		
	vercond_list = (VersionCondition **)malloc(len*sizeof(VersionCondition *));
	if (!vercond_list) {
		*errmsg = CFSTR("Failed to allocate an array of VersionCondition");
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
		*errmsg = CFSTR("Failed to allocate VersionConditionSet");
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
     
void VersionConditionSetFree(VersionConditionSet *vercond_set)
{
	if (!vercond_set) return;
	VersionCondition **vclist = vercond_set->conditions;
	for (CFIndex n = 0; n < vercond_set->length; n++) {
		VersionConditionFree(vclist[n]);
	}
	free(vclist);
}

Boolean VersionConditionSetIsSatisfied(VersionConditionSet *vercond_set, CFStringRef version)
{
	VersionCondition **vclist = vercond_set->conditions;
	for (CFIndex n = 0; n < vercond_set->length; n++) {
		if (!VersionConditionIsSatisfied(vclist[n], version)) return false;
	}
	return true;
}