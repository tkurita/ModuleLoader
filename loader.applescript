﻿property name : "LoaderProxy"

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
property PropertyAccessor : (run script POSIX file (cwd() & "PropertyAccessor.applescript"))'s initialize()

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
	try
		resolve_dependencies(a_script, __module_dependencies__ of a_script, false)
	end try
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



on export(a_script) -- save myself to cache when load a module which load myself.
	export_to_cache(name of a_script, a_script)
end export

on export_to_cache(a_name, a_script)
	my _module_cache's add_module(a_name, missing value, a_script)
end export_to_cache

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
	
	set adpaths to my _additional_paths
	if (my _is_local and (length of adpaths is 0)) then
		set adpaths to {current_location()}
	end if
	
	if my _collecting or my _only_local then
		try
			set a_script to _load module_ a_name additional paths adpaths without other paths
		on error msg number errno
			if my _collecting then
				set a_script to try_collect(a_name, adpaths)
			else
				error msg number errno
			end if
		end try
	else
		set a_script to _load module_ a_name additional paths adpaths
	end if
	
	set a_script to _load module_ a_name additional paths adpaths
	
	
	set a_path to __module_path__ of a_script
	
	try
		set a_script to my _module_cache's module_for_path(a_path)
		my _module_cache's add_module(a_name, a_path, a_script)
		set need_setup to false
	on error number 900
		set need_setup to true
	end try
	if need_setup then
		--do_log("did not hit in script chache")
		my _module_cache's add_module(a_name, a_path, a_script)
		if not my _loadonly then
			setup_script(a_script)
		end if
	else
		--do_log("hit in script chache")
	end if
	
	return a_script
end load_module

on resolve_dependencies(a_script, dependencies, is_top)
	repeat with a_dep in dependencies
		set an_accessor to PropertyAccessor's make_with_name(name of a_dep)
		set a_module to load_module(name of module specifier of a_dep)
		an_accessor's set_value(a_script, a_module)
		log a_module
		if is_top then
			set __module_specifier__ of a_module to module specifier of a_dep
		end if
	end repeat
end resolve_dependencies

on drive for a_script
	set dependencies to extract dependencies for a_script
	resolve_dependencies(a_script, dependencies, true)
	return missing value
end drive

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

on try_collect(a_name, adpaths)
	set a_script to _load module_ a_name additional paths adpaths
	set a_path to __module_path__ of a_script
	set a_location to item 1 of adpaths
	tell application "Finder"
		set new_alias to make alias file at a_location to a_path -- with properties {name:name of path_rec}
	end tell
	return a_script
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
(*
property ModuleA : module "Module A"
on run
	--drive (module loader) for me
	drive for me
end run
*)