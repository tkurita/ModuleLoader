#include <CoreFoundation/CoreFoundation.h>
#include "ModuleCondition.h"
#include "AEUtils.h"

CFStringRef CFStringCreateWithEscapingRegex(CFStringRef text, CFStringRef *errmsg)
{
	static TXRegularExpression *regexp = NULL;
	UErrorCode status = U_ZERO_ERROR;
	if (!regexp) {
		UParseError parse_error;
		regexp = TXRegexCreate(CFSTR("([\\.\\+\\(\\)\\*\\?])"), 0, &parse_error, &status);
		if (U_ZERO_ERROR != status) {
			CFStringRef parse_error_msg = CFStringCreateWithFormattingParseError(&parse_error);
			*errmsg = CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
						   CFSTR("Failed to compile regular expression. \n%@"), parse_error_msg);
			if (!parse_error_msg) CFRelease(parse_error_msg);
			return NULL;
		}
	}
	CFStringRef result = CFStringCreateByReplacingAllMatches(text, regexp, CFSTR("\\$1"), &status);
	if (U_ZERO_ERROR != status) {
		*errmsg = CFStringCreateWithFormat(kCFAllocatorDefault, NULL,
										  CFSTR("Failed to regex escaping with error : %d"), status);
		return NULL;
	}
	return result;
}

ModuleCondition *ModuleConditionCreate(CFStringRef module_name, CFStringRef required_version, CFStringRef *errmsg)
{
	CFStringRef escaped_name = NULL;
	CFStringRef verpattern = NULL;
	CFArrayRef subpath = NULL;
	ModuleCondition *module_condition = NULL;
	
	CFRange colon_range  = CFStringFind(module_name, CFSTR(":"), 0);
	if (colon_range.location != kCFNotFound) {
		subpath = CFStringCreateArrayBySeparatingStrings(NULL, module_name, CFSTR(":"));
		module_name = CFArrayGetValueAtIndex(subpath, CFArrayGetCount(subpath)-1);
	}
	
	module_condition = malloc(sizeof(ModuleCondition));
	if (!module_condition) {
		fprintf(stderr, "Failed to allocate ModuleCondition.\n");
		goto bail;;
	}
	module_condition->required_version = NULL;
	module_condition->pattern = NULL;
	module_condition->subpath = NULL;
	module_condition->name = CFRetain(module_name);
	if (required_version) {
		module_condition->required_version = VersionConditionSetCreate(required_version, errmsg);
	} else {
		module_condition->required_version = NULL;
	}
	
	
	if (! (escaped_name = CFStringCreateWithEscapingRegex(module_name, errmsg))) {
		ModuleConditionFree(module_condition);
		goto bail;
	}
	
	verpattern = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, 
								CFSTR("%@(-[0-9\\.]+)?(\\.(scptd|scpt|applescript|app))?"), escaped_name);
	UParseError parse_error;
	UErrorCode status = U_ZERO_ERROR;
	module_condition->pattern = TXRegexCreate(verpattern, UREGEX_CASE_INSENSITIVE, &parse_error, &status);
	if (U_ZERO_ERROR != status) {
		ModuleConditionFree(module_condition);
		module_condition = NULL;
	}
	
bail:	
	safeRelease(escaped_name);
	safeRelease(verpattern);
	return module_condition;
}

void ModuleConditionFree(ModuleCondition *module_condition)
{
	if (!module_condition) return;
	safeRelease(module_condition->name);
	safeRelease(module_condition->subpath);
	VersionConditionSetFree(module_condition->required_version);
	TXRegexFree(module_condition->pattern);
}

Boolean ModuleConditionVersionIsSatisfied(ModuleCondition *module_condition, ModuleRef *module_ref)
{
	if (!module_condition->required_version) return true;
	CFStringRef version = ModuleRefGetVersion(module_ref);
	if (!version) return false;
	return VersionConditionSetIsSatisfied(module_condition->required_version, version);
}

Boolean ModuleConditionHasSubpath(ModuleCondition *module_condition)
{
	return (module_condition->subpath != NULL);
}

CFArrayRef ModuleConditionParseName(ModuleCondition *module_condition, CFStringRef name)
{
	UErrorCode status = U_ZERO_ERROR;
	CFArrayRef result = CFStringCreateArrayWithFirstMatch(name, module_condition->pattern, 0, &status);
	if (U_ZERO_ERROR != status) {
		safeRelease(result);
		result = NULL;
	}
	return result;
}
