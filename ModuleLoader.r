#include <Carbon/Carbon.r>

#define Reserved8   reserved, reserved, reserved, reserved, reserved, reserved, reserved, reserved
#define Reserved12  Reserved8, reserved, reserved, reserved, reserved
#define Reserved13  Reserved12, reserved
#define dp_none__   noParams, "", directParamOptional, singleItem, notEnumerated, Reserved13
#define reply_none__   noReply, "", replyOptional, singleItem, notEnumerated, Reserved13
#define synonym_verb__ reply_none__, dp_none__, { }
#define plural__    "", {"", kAESpecialClassProperties, cType, "", reserved, singleItem, notEnumerated, readOnly, Reserved8, noApostrophe, notFeminine, notMasculine, plural}, {}

resource 'aete' (0, "ModuleLoader Terminology") {
	0x1,  // major version
	0x0,  // minor version
	english,
	roman,
	{
		"Type Names Suite",
		"Hidden terms",
		kASTypeNamesSuite,
		1,
		1,
		{
			/* Events */

		},
		{
			/* Classes */

			"script", 'scpt',
			"",
			{
			},
			{
			}
		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		},

		"ModuleLoader Suite",
		"loding script module",
		'Molo',
		1,
		1,
		{
			/* Events */

			"find module",
			"Find module from module paths. If specified module can not be forund, the error number 1800 will raise.",
			'Molo', 'fdMo',
			'file',
			"A reference to a module.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'TEXT',
			"A module name",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"additional paths", 'inDr', 'file',
				"Additional locations to search modules.",
				optional,
				listOfItems, notEnumerated, Reserved13,
				"other paths", 'ohPh', 'bool',
				"If tue is passed,  module search paths are restricted to  paths given in 'additional paths'.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"load module",
			"Load a module from module paths. If specified module can not be forund, the error number 1800 will raise.",
			'Molo', 'loMo',
			'scpt',
			"A loaded module.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'TEXT',
			"A module name.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"additional paths", 'inDr', 'file',
				"Additional locations to search modules.",
				optional,
				listOfItems, notEnumerated, Reserved13,
				"other paths", 'ohPh', 'bool',
				"If tue is passed,  module search paths are restricted to  paths given in 'additional paths'.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"make loader",
			"Obtain a loader script",
			'Molo', 'MKlo',
			'scpt',
			"A loader script",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"make local loader",
			"Obtain a local loader script. The Local loader search modules not only module paths but also \"path to me\" location.",
			'Molo', 'MkLl',
			'scpt',
			"A loader script",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"module paths",
			"List module paths. The default paths are ~/Library/Scritps/Modules and /Library/Scripts/Modules. Only existing folders are listed. Additional paths given by \"set additional paths to\" will be included in the result.",
			'Molo', 'gtPH',
			'file',
			"List of POSIX paths of folders.",
			replyRequired, listOfItems, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"set additional module paths to",
			"Prepend module search paths.",
			'Molo', 'adMp',
			'bool',
			"If success return true",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'file',
			"Folders including script modules",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"ModuleLoader version",
			"get version number of ModuleLoader",
			'Molo', 'Vers',
			'TEXT',
			"version number",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"construct module",
			"A handler for a module script to construct a script on loaded time. This event is sent by a loader script when a script is loaded. Return a script constucted in this event.",
			'Molo', 'Csrt',
			'scpt',
			"A script constructed on loaded time.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'scpt',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"module loaded",
			"A handler in which a module resolve dependencies. This event is sent by a loader script when a script is loaded. ",
			'Molo', 'wlLd',
			reply_none__,
			dp_none__,
			{
				"by", 'whLD', 'scpt',
				"A loader script",
				required,
				singleItem, notEnumerated, Reserved13
			}
		},
		{
			/* Classes */

		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		}
	}
};
