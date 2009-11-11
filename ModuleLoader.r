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
			"load module",
			'Molo', 'loMo',
			'****',
			"script object",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'TEXT',
			"URL",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"in directory", 'inDr', 'file',
				"specify script module location",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"set additional module pathes",
			"prepend module search pathes to default pathes",
			'Molo', 'adMp',
			'bool',
			"if success return true",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'file',
			"the directory including script modules",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"module pathes",
			"list module pathes",
			'Molo', 'gtPH',
			'file',
			"list of directory",
			replyRequired, listOfItems, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"find module",
			"find module",
			'Molo', 'fdMo',
			'file',
			"",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'TEXT',
			"A module name",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"make loader",
			"Obtain a loader script",
			'Molo', 'MKlo',
			'****',
			"",
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
