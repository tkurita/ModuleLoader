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
		"ModuleLoader Suite",
		"loding script module",
		'Molo',
		1,
		1,
		{
			/* Events */

			"load module",
			"Load a module from module paths. If specified module can not be forund, the error number 1800 will raise.",
			'Molo', 'loMo',
			'****',
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

			"module paths",
			"List module paths. The default paths are ~/Library/Scritps/Modules and /Library/Scripts/Modules. Only existing folders are listed. Additional paths given by \"set additional paths to\" will be included in the result.",
			'Molo', 'gtPH',
			'file',
			"List of POSIX paths of folders.",
			replyRequired, listOfItems, notEnumerated, Reserved13,
			dp_none__,
			{

			},

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

			"make loader",
			"Obtain a loader script",
			'Molo', 'MKlo',
			'****',
			"A loader script",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"make local loader",
			"Obtain a local loader script. The Local loader search modules not only module paths but also \"path to me\" location.",
			'Molo', 'MkLl',
			'****',
			"A loader script",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
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

			"will loaded",
			"Sent by a loader script when a script is loaded.",
			'Molo', 'wlLd',
			reply_none__,
			'****',
			"A loader script",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"construct",
			"Sent by a loader script when a script is loaded. Return a script constucted in this event.",
			'Molo', 'Csrt',
			'****',
			"A script constructed on loaded time.",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

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
