property name : "LoaderProxy"
property ModuleCache : missing value
property ConsoleLog : missing value

on __load__(loader)
	tell loader
		set ModuleCache to load("ModuleCache")
		set ConsoleLog to load("ConsoleLog")
	end tell
end __load__

property _ : __load__((proxy() of application (get "ModuleLoaderLib"))'s set_localonly(true))

property _loadonly : false
--property _setuped_scripts : make XDict
--property _exported_modules : make XDict
property _module_cache : make ModuleCache
property _logger : missing value

(** Properties for local loader **)
property _is_local : false
property _additional_paths : {}
property _collecting : false
property _only_local : false

on setup_script(a_script)
	do_log("start setup_script")
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
			do_log("error on calling __load__")
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

on find_module(a_name, a_location)
	set loc_path to quoted form of POSIX path of a_location
	set a_result to do shell script "module_name=`echo '" & a_name & "'|/usr/bin/iconv -f UTF-8 -t UTF-8-MAC`; find -E " & loc_path & " -regex " & quote & "(.*/)*$module_name($|.scpt|.scptd|.app)$" & quote
	if a_result is "" then
		raise_error(a_name, a_location)
	end if
	set a_path to (POSIX file (paragraph 1 of a_result)) as alias
	return a_path
end find_module

on export(a_script) -- save myself to cache when load a module which load myself.
	export_to_cache(a_script)
end export

on export_to_cache(a_script)
	--my _exported_modules's set_value(name of a_script, a_script)
	my _module_cache's add_module(name of a_script, missing value, a_script)
end export_to_cache

on load module a_name
	return load(a_name)
end load module

on load_module(a_name)
	return load(a_name)
end load_module

on load(a_name)
	do_log("start load for " & quoted form of a_name)
	if a_name is in {":", "", "/", "."} then
		error (quoted form of a_name) & " is invald form to specify a module." number 1801
	end if
	try
		--set a_script to my _exported_modules's value_for_key(a_name)
		set a_script to my _module_cache's module_for_name(a_name)
		set has_exported to true
	on error number 900
		set has_exported to false
	end try
	if has_exported then
		return a_script
	end if
	
	set adpaths to my _additional_paths
	if (my _is_local and (length of adpaths is 0)) then
		set adpaths to {current_location()}
	end if
	
	if my _collecting or my _only_local then
		try
			set a_path to find_module(a_name, item 1 of adpaths)
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
	
	try
		--set a_script to my _setuped_scripts's value_for_key(a_path)
		set a_script to my _setuped_scripts's module_for_path(a_path)
		my _module_cache's add_module(a_name, a_path, a_script)
		set need_setup to false
	on error number 900
		set need_setup to true
	end try
	if need_setup then
		do_log("did not hit in script chache")
		set a_script to load script a_path
		--my _setuped_scripts's set_value(a_path, a_script)
		--my _exported_modules's set_value(a_name, a_script)
		my _module_cache's add_module(a_name, a_path, a_script)
		if not my _loadonly then
			setup_script(a_script)
		end if
	else
		do_log("hit in script chache")
	end if
	
	return a_script
end load

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
	set my _exported_modules to missing value
	set my _setuped_scripts to missing value
end clear_cache

(*
on start_log()
	ConsoleLog's start_log()
end start_log

on stop_log()
	ConsoleLog's stop_log()
end stop_log
*)

