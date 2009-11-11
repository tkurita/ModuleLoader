property name : "LoaderProxy"
property XDict : missing value
property ConsoleLog : missing value
property XAccessor : missing value

on __load__(loader)
	do shell script "syslog -l 5 -s 'start __load__ in loader'"
	tell loader
		set XDict to load("XDict")
		set ConsoleLog to load("ConsoleLog")
		set XAccessor to load("XAccessor")
	end tell
end __load__

property _ : __load__(proxy() of application (get "ModuleLoaderLib"))

property _autocollect : false
property _loadonly : false
property _setuped_scripts : make XDict
property _path_cache : make XDict
property _exported_modules : make XDict
property _global_accessors : make XDict
property _original_name : name
property _logger : missing value
property _from_original : true

on setup_script(a_script)
	do_log("start setup_script")
	try
		a_script's __load__(me)
	on error msg number errno
		do_log("error on calling __load__")
		-- 1800 : the module is not found
		-- -1708 : handelr __load__ is not implemented
		if errno is not -1708 then
			error msg number errno
		end if
		--display dialog msg & return & errno
	end try
	
	try
		set a_buffer to a_script's __construct__()
		set a_script to a_buffer
	on error msg number errno
		do_log("error on calling __construct__")
		-- 1800 : the module is not found
		-- -1708 : handelr __construct__ is not implemented
		if errno is not -1708 then
			error msg number errno
		end if
		--display dialog msg & return & errno
	end try
	--return a_script
end setup_script

on raise_error(a_name)
	set folder_path to quoted form of (my _location as Unicode text)
	error a_name & " is not found in " & folder_path number 1800
end raise_error

on try_collect(a_name)
	set original_loader to proxy() of application (my _original_name)
	set a_path to original_loader's find_module(a_name)
	tell application "Finder"
		set new_alias to make alias file at my _location to a_path -- with properties {name:name of path_rec}
	end tell
	return new_alias as Unicode text
end try_collect

on do_log(msg)
	if my _logger is not missing value then
		my _logger's do(msg)
	end if
end do_log

on find_module(a_name)
	try
		set a_path to my _path_cache's value_for_key(a_name)
		set no_cached to false
	on error number 900
		set no_cached to true
	end try
	if no_cached then
		do_log("did not hit in path chache")
		(*
		set loc_path to quoted form of POSIX path of my _location
		set a_result to do shell script "module_name=`echo '" & a_name & "'|/usr/bin/iconv -f UTF-8 -t UTF-8-MAC`; find -E " & loc_path & " -regex " & quote & "(.*/)*$module_name($|.scpt|.scptd|.app)$" & quote
		*)
		set a_result to find module a_name
		set a_path to POSIX path of a_result
		--set a_path to (POSIX file (paragraph 1 of a_result)) as Unicode text
		my _path_cache's set_value(a_name, a_path)
	else
		do_log("hit in path chache")
	end if
	
	return a_path
end find_module

on export(a_script) -- save myself to cache when load a module which load myself.
	export_to_cache(a_script)
end export

on global_accessor(a_name)
	try
		set an_accessor to my _global_accessors's value_for_key(a_name)
		set no_accessor to false
	on error number 900
		set no_accessor to true
	end try
	if no_accessor then
		set an_accessor to XAccessor's make_with(a_name)
		my _global_accessors's set_value(a_name, an_accessor)
	end if
	return an_accessor
end global_accessor

on export_to_global(a_script)
	set an_accessor to global_accessor(name of a_script)
	an_accessor's set_global(a_script)
end export_to_global

on export_to_cache(a_script)
	my _exported_modules's set_value(name of a_script, a_script)
end export_to_cache

on require(a_name)
	set an_accessor to global_accessor(a_name)
	try
		set a_script to an_accessor's get_global()
	on error
		set a_script to load(a_name)
		an_accessor's set_global(a_script)
		return
	end try
	
	if a_script is missing value then
		set a_script to load(a_name)
		an_accessor's set_global(a_script)
	end if
end require

on load(a_name)
	do_log("start load for " & quoted form of a_name)
	if a_name is in {":", "", "/", "."} then
		error (quoted form of a_name) & " is invald form to specify a module." number 1801
	end if
	try
		set a_script to my _exported_modules's value_for_key(a_name)
		set has_exported to true
	on error number 900
		set has_exported to false
	end try
	if has_exported then
		return a_script
	end if
	
	try
		set a_path to find_module(a_name)
	on error errmsg number errnum
		if (not my _from_original) and (my _autocollect) then
			set a_path to try_collect(a_name)
		else
			error errmsg number errnum
		end if
	end try
	
	try
		set a_script to my _setuped_scripts's value_for_key(a_path)
		set no_setuped to false
	on error number 900
		set no_setuped to true
	end try
	if no_setuped then
		do_log("did not hit in script chache")
		set a_script to load script (POSIX file a_path)
		my _setuped_scripts's set_value(a_path, a_script)
		my _exported_modules's set_value(a_name, a_script)
		if not my _loadonly then
			setup_script(a_script)
		end if
	else
		do_log("hit in script chache")
	end if
	
	return a_script
end load

on set_autocollect(a_flag)
	set my _autocollect to a_flag
end set_autocollect

on set_loadonly(a_flat)
	set my _loadonly to a_flag
end set_loadonly

on set_logging(a_flag, loader_name)
	if a_flag then
		set my _logger to ConsoleLog's make_with(loader_name)
	else
		set my _logger to missing value
	end if
end set_logging

on debug {}
	global _isOriginal
	set _isOriginal to false
	set a_loader to make_for_location(path to scripts folder)
	a_loader's load("XList")
	--a_loader's load("Modules:FileObject.scptd")
end debug

(*
on start_log()
	ConsoleLog's start_log()
end start_log

on stop_log()
	ConsoleLog's stop_log()
end stop_log
*)

