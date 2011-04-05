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
		"loding script modules",
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
				singleItem, notEnumerated, Reserved13,
				"version", 'vers', 'TEXT',
				"Required version.",
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
				singleItem, notEnumerated, Reserved13,
				"version", 'vers', 'TEXT',
				"Required version.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"_load module_",
			"A command for private use.",
			'Molo', 'PloM',
			'reco',
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
				singleItem, notEnumerated, Reserved13,
				"version", 'vers', 'TEXT',
				"Required version.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"module loader",
			"Obtain a loader object which is a script object to manage loaded modules.",
			'Molo', 'MKlo',
			'scpt',
			"A loader object",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"_module loader_",
			"A command for private use.",
			'Molo', 'PMKl',
			'scpt',
			"A loader object",
			replyRequired, singleItem, notEnumerated, Reserved13,
			dp_none__,
			{

			},

			"make local loader",
			"Obtain a local loader object. The local loader search modules not only module paths but also \"path to me\" location.",
			'Molo', 'MkLl',
			'scpt',
			"A loader object",
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

			"module",
			"Make a module specifier record. Must be placed in a user defined propery statement or in an argument of load handler of a loader object. If a module name is ommited, property name is used as a module name.",
			'Molo', 'MkMs',
			'MoSp',
			"A module specifer record",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'TEXT',
			"A module name.",
			directParamOptional,
			singleItem, notEnumerated, Reserved13,
			{
				"reloading", 'pRLo', 'bool',
				"Whether or not to load a module ignoring module chache.",
				optional,
				singleItem, notEnumerated, Reserved13,
				"version", 'vers', 'TEXT',
				"Required version.",
				optional,
				singleItem, notEnumerated, Reserved13
			},

			"extract dependencies from",
			"Extarct module specifier records from a module script.",
			'Molo', 'Exdp',
			'****',
			"",
			replyRequired, listOfItems, notEnumerated, Reserved13,
			'scpt',
			"A module script",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{

			},

			"boot",
			"Set up modules depending on. This command must be sent to a loader object. returned from \"module loader\" command.",
			'Molo', 'Boot',
			'scpt',
			"",
			replyRequired, singleItem, notEnumerated, Reserved13,
			'scpt',
			"A loader script generated by make loader command.",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"for", 'forM', 'scpt',
				"A top-level script which need to load modules",
				required,
				singleItem, notEnumerated, Reserved13
			},

			"module loaded",
			"Called when a module is loaded by a loader object. It's a place to customize modules.",
			'Molo', 'wlLd',
			reply_none__,
			'scpt',
			"",
			directParamRequired,
			singleItem, notEnumerated, Reserved13,
			{
				"by", 'whLD', 'scpt',
				"A loader object",
				optional,
				singleItem, notEnumerated, Reserved13
			}
		},
		{
			/* Classes */

			"module specifier", 'MoSp',
			"A specifier of a module will be loaded.",
			{
				"name", 'pnam', 'TEXT',
				"Module name",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"reloading", 'pRLo', 'bool',
				"Whether or not to load a module ignoring module chache.",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"version", 'vers', 'TEXT',
				"Required version.",
				reserved, singleItem, notEnumerated, readWrite, Reserved12
			},
			{
			},

			"dependency info", 'DpIf',
			"Module dependency information",
			{
				"name", 'pnam', 'TEXT',
				"A property name which a module is loaded.",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"module specifier", 'MoSp', 'MoSp',
				"A module specifier",
				reserved, singleItem, notEnumerated, readWrite, Reserved12
			},
			{
			},

			"local loader options", 'OPll',
			"",
			{
				"collecting modules", 'cLMd', 'bool',
				"",
				reserved, singleItem, notEnumerated, readWrite, Reserved12,

				"only local", 'oNLo', 'bool',
				"",
				reserved, singleItem, notEnumerated, readWrite, Reserved12
			},
			{
			}
		},
		{
			/* Comparisons */
		},
		{
			/* Enumerations */
		}
	}
};
