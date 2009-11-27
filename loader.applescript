property name : "LoaderProxy"

on cwd()
	set pwd to system attribute "PWD"
	if pwd is "" or pwd is ((path to startup disk)'s POSIX path) then
		return "/Users/tkurita/Dev/Projects/ModuleLoader/trunk/"
	else
		return pwd & "/"
	end if
end cwd

property ModuleCache : run script POSIX file (cwd() & "ModuleCache.applescript")
property XList : run script POSIX file (cwd() & "FastList.applescript")
property ConsoleLog : run script POSIX file (cwd() & "ConsoleLog.applescript")

property _loadonly : false
property _module_cache : make ModuleCache
property _logger : missing value

(** Properties for local loader **)
property _is_local : false
property _additional_paths : {}
property _collecting : false
property _only_local : false

on setup_script(a_script)
	--do_log("start setup_script")
	set sucess_setup to false
	try
		module loaded a_script by me
		set sucess_setup to true
	on error msg number errno
		-- 1800 : the module is not found
		-- -1708 : handelr "module loaded" is not implemented
		if errno is not -1708 then
			error msg number errno
		end if
	end try
	
	if (not sucess_setup) then
		-- for compatibility to ModuleLoader 1.x
		try
			a_script's __load__(me)
		on error msg number errno
			--do_log("error on calling __load__")
			if errno is not -1708 then
				error msg number errno
			end if
			--display dialog msg & return & errno
		end try
	end if
end setup_script

on raise_error(a_name, a_location)
	set folder_path to quoted form of (a_location as Unicode text)
	error a_name & " is not found in " & folder_path number 1800
end raise_error

on do_log(msg)
	if my _logger is not missing value then
		my _logger's do(msg)
	end if
end do_log

on find_without_osax(a_name, a_location)
	set module_path to missing value
	if a_name contains ":" then
		set loc_path to POSIX path of a_location
		set delim to AppleScript's text item delimiters
		set AppleScript's text item delimiters to ":"
		set path_elems to every text item of a_name
		set AppleScript's text item delimiters to delim
		set mod_name to last item of path_elems
		if (length of mod_name < 1) then
			error (quoted form of a_name) & " is invald form to specify a module." number 1801
		end if
		repeat with n from 1 to (length of path_elems) - 1
			set an_elem to item 1 of path_elems
			if (length of an_elem > 0) then
				set loc_path to loc_path & (an_elem) & "/"
			end if
		end repeat
		tell application "System Events"
			set a_folder to item loc_path
			tell a_folder
				set module_items to items whose name starts with mod_name
			end tell
		end tell
		if (length of module_items < 1) then
			raise_error(a_name, a_location)
		end if
		tell application "System Events"
			repeat with an_item in module_items
				if name extension of an_item is in {"scpt", "scptd", "app"} then
					set module_path to an_item as alias
					exit repeat
				end if
			end repeat
		end tell
	else
		set loc_path to quoted form of POSIX path of a_location
		set a_result to do shell script "module_name=`echo '" & a_name & "'|/usr/bin/iconv -f UTF-8 -t UTF-8-MAC`; find -E " & loc_path & " -regex " & quote & "(.*/)*$module_name($|.scpt|.scptd|.app)$" & quote
		if a_result is not "" then
			set module_path to (POSIX file (paragraph 1 of a_result)) as alias
		end if
	end if
	
	if module_path is missing value then
		raise_error(a_name, a_location)
	end if
	return module_path
end find_without_osax

on export(a_script) -- save myself to cache when load a module which load myself.
	export_to_cache(name of a_script, a_script)
end export

on export_to_cache(a_name, a_script)
	my _module_cache's add_module(a_name, missing value, a_script)
end export_to_cache

on find_module(a_name) -- clients should call only for debugging
	set adpaths to my _additional_paths
	if (my _is_local and (length of adpaths is 0)) then
		set adpaths to {current_location()}
	end if
	
	if my _collecting or my _only_local then
		try
			set a_path to find_without_osax(a_name, item 1 of adpaths)
		on error msg number errno
			if my _collecting then
				set a_path to try_collect(a_name, item 1 of adpaths)
			else
				error msg number errno
			end if
		end try
	else
		set a_path to find module a_name additional paths adpaths
	end if
	
	return a_path
end find_module

on load module a_name
	return load_module(a_name)
end load module

on load(a_name)
	return load_module(a_name)
end load

on load_module(a_name)
	--do_log("start load for " & quoted form of a_name)
	if a_name is in {":", "", "/", "."} then
		error (quoted form of a_name) & " is invald form to specify a module." number 1801
	end if
	try
		set a_script to my _module_cache's module_for_name(a_name)
		set has_exported to true
	on error number 900
		set has_exported to false
	end try
	if has_exported then
		return a_script
	end if
	
	set a_path to find_module(a_name)
	
	try
		set a_script to my _module_cache's module_for_path(a_path)
		my _module_cache's add_module(a_name, a_path, a_script)
		set need_setup to false
	on error number 900
		set need_setup to true
	end try
	if need_setup then
		--do_log("did not hit in script chache")
		set a_script to load script a_path
		my _module_cache's add_module(a_name, a_path, a_script)
		if not my _loadonly then
			setup_script(a_script)
		end if
	else
		--do_log("hit in script chache")
	end if
	
	return a_script
end load_module

on set_loadonly(a_flat)
	set my _loadonly to a_flag
	return me
end set_loadonly

on set_logging(a_flag, loader_name)
	if a_flag then
		set my _logger to ConsoleLog's make_with(loader_name)
	else
		set my _logger to missing value
	end if
	
	return a reference to me
end set_logging

(** AppleMods handler **)
on loadLib(a_name)
	return load_module(a_name)
end loadLib

(** Handlers for local loader **)

on set_localonly(a_flag)
	set my _only_local to a_flag
	return me
end set_localonly

on collecting_modules(a_flag)
	set my _collecting to a_flag
	return me
end collecting_modules

on current_location()
	set a_path to path to me
	tell application "Finder"
		set a_folder to folder of a_path as alias
	end tell
	return a_folder
end current_location

on set_additional_paths(a_list)
	set my _additional_paths to a_list
end set_additional_paths

on set_local(a_flag)
	set my _is_local to true
	return me
end set_local

on try_collect(a_name, a_location)
	set a_path to find module a_name
	tell application "Finder"
		set new_alias to make alias file at a_location to a_path -- with properties {name:name of path_rec}
	end tell
	return new_alias as alias
end try_collect

on clear_cache()
	set my _module_cache to missing value
end clear_cache

(*
on start_log()
	ConsoleLog's start_log()
end start_log

on stop_log()
	ConsoleLog's stop_log()
end stop_log
*)

