property name : "LoaderProxy"

on import(a_name)
	set pwd to system attribute "PWD"
	if pwd is "" or pwd is ((path to startup disk)'s POSIX path) then
		set pwd to "/Users/tkurita/Dev/Projects/ModuleLoader/trunk/"
	else
		set pwd to pwd & "/"
	end if
	
	return run script POSIX file (pwd & a_name & ".applescript")
end import

property FastList : import("FastList")
property ModuleCache : import("ModuleCache")'s initialize()
property ConsoleLog : import("ConsoleLog")
property PropertyAccessor : import("PropertyAccessor")'s initialize()
property ModuleInfo : import("ModuleInfo")

property _loadonly : false
property _module_cache : make ModuleCache
--property _logger : ConsoleLog's make_with("ModuleLoader")'s start_log()
property _logger : missing value

(** Properties for local loader **)
property _is_local : false
property _additional_paths : {}
property _collecting : false
property _only_local : false

on setup_script(a_moduleinfo)
	set a_script to a_moduleinfo's module_script()
	--log ("start setup_script" & " for " & name of a_script)
	a_moduleinfo's set_setupped(true)
	resolve_dependencies(a_moduleinfo, false)
	set sucess_setup to false
	try
		--log "before module loaded : " & (name of a_script)
		module loaded a_script by me
		set sucess_setup to true
	on error msg number errno
		-- 1800 : the module is not found
		-- -1708 : handelr "module loaded" is not implemented
		if errno is not -1708 then
			error msg number errno
		end if
	end try
	--log "end setup_script"
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
	set a_moduleinfo to ModuleInfo's make_with_vars(a_script, {}, true)
	my _module_cache's add_module(a_name, missing value, a_moduleinfo)
end export_to_cache

on load module mspec
	return load(mspec)
end load module

on load(mspec)
	set a_moduleinfo to load_module(mspec)
	if a_moduleinfo's need_setup() then
		if not my _loadonly then
			setup_script(a_moduleinfo)
		end if
	end if
	return a_moduleinfo's module_script()
end load

on current_location()
	set a_path to path to me
	tell application "Finder"
		set a_folder to container of a_path as alias
	end tell
	return a_folder
end current_location

on load_module(mspec)
	--log "start load_module"
	set force_reload to false
	set a_class to class of mspec
	set required_version to missing value
	if a_class is in {record, module specifier} then
		set a_name to mspec's name
		try
			set force_reload to reloading of mspec
		end try
		try
			set required_version to mspec's version
		end try
	else if a_class is list then
		set a_name to item 1 of mspec
		try
			set force_reload to reloading of item 2 of mspec
		end try
	else
		set a_name to mspec
	end if
	
	if a_name is in {":", "", "/", "."} then
		error (quoted form of mspec's name) & " is invald form to specify a module." number 1801
	end if
	
	if not force_reload then
		try
			if required_version is missing value then
				set a_moduleinfo to my _module_cache's module_for_name(a_name)
			else
				set a_moduleinfo to my _module_cache's module_for_name_version(a_name, required_version)
			end if
			set has_exported to true
		on error number 900
			set has_exported to false
		end try
		
		if has_exported then
			--log "end load_module with has_exported"
			return a_moduleinfo
		end if
	end if
	
	set adpaths to my _additional_paths
	if (my _is_local and ((count adpaths) is 0)) then
		set adpaths to {current_location()}
	end if
	
	if my _collecting or my _only_local then
		try
			set a_loadinfo to _load module_ mspec additional paths adpaths without other paths
		on error msg number errno
			if my _collecting then
				set a_loadinfo to try_collect(mspec, adpaths)
			else
				error msg number errno
			end if
		end try
	else
		set a_loadinfo to _load module_ mspec additional paths adpaths
	end if
	--log "after _load_module_"
	
	set a_path to file of a_loadinfo
	--log a_path
	if force_reload then
		set a_moduleinfo to ModuleInfo's make_with_loadinfo(a_loadinfo)
		my _module_cache's replace_module(a_name, a_path, a_moduleinfo)
	else
		try
			set a_moduleinfo to my _module_cache's module_for_path(a_path)
			my _module_cache's add_module(a_name, a_path, a_moduleinfo)
		on error number 900
			set a_moduleinfo to ModuleInfo's make_with_loadinfo(a_loadinfo)
			my _module_cache's add_module(a_name, a_path, a_moduleinfo)
		end try
	end if
	--log "end of load_module"
	return a_moduleinfo
end load_module

on resolve_dependencies(a_moduleinfo)
	--log "start reslove dependencies"
	repeat with a_dep in a_moduleinfo's dependencies()
		set an_accessor to PropertyAccessor's make_with_name(name of a_dep)
		set dep_moduleinfo to load_module(module specifier of a_dep)
		if dep_moduleinfo's need_setup() then
			setup_script(dep_moduleinfo)
		end if
		an_accessor's set_value(a_moduleinfo's module_script(), dep_moduleinfo's module_script())
	end repeat
	--log "end reslove dependencies"
end resolve_dependencies

on boot loader for a_script
	global __module_dependencies__
	
	-- options for local loader
	if my _is_local then
		try
			if class of collecting modules of a_script is boolean then -- avoid problem in osacompile Mac OS X 10.6
				set my _collecting to collecting modules of a_script
			end if
		end try
		try
			if class of only local of a_script is boolean then -- avoid problem in osacompile Mac OS X 10.6
				set my _only_local to only local of a_script
			end if
		end try
	end if
	
	try
		set dependencies to __module_dependencies__
		--log "found __module_dependencies__"
	on error
		set dependencies to extract dependencies from a_script
		--log "not found __module_dependencies__"
	end try
	
	set moduleinfo_list to {}
	repeat with a_dep in dependencies
		--log name of a_dep
		set a_moduleinfo to load_module(module specifier of a_dep)
		set an_accessor to PropertyAccessor's make_with_name(name of a_dep)
		an_accessor's set_value(a_script, a_moduleinfo's module_script())
		set end of moduleinfo_list to a_moduleinfo
	end repeat
	
	repeat with a_moduleinfo in moduleinfo_list
		if a_moduleinfo's need_setup() then
			setup_script(a_moduleinfo)
		end if
	end repeat
	
	try
		module loaded a_script by me
	on error msg number errno
		-- 1800 : the module is not found
		-- -1708 : handelr "module loaded" is not implemented
		if errno is not -1708 then
			error msg number errno
		end if
	end try
	--log "will set __module_dependencies__"
	set __module_dependencies__ to dependencies
	return loader
end boot

(** for paths **)
on set_additional_paths(a_list)
	set my _additional_paths to a_list
	return me
end set_additional_paths

on prepend_path(a_path)
	if my _additional_paths is missing value then
		set my _additional_paths to {a_path}
	else
		set my _additional_paths to (a_path as list) & my _additional_paths
	end if
	return me
end prepend_path

on module_paths()
	return my _additional_paths & (module paths)
end module_paths

(** Handlers for local loader **)

on set_localonly(a_flag)
	set my _only_local to a_flag
	return me
end set_localonly

on collecting_modules(a_flag)
	set my _collecting to a_flag
	return me
end collecting_modules

on set_local(a_flag)
	set my _is_local to true
	return me
end set_local

(** misc **)
on module_version_of(a_script)
	try
		set a_moduleinfo to my _module_cache's module_for_script(a_script)
	on error number 900
		return missing value
	end try
	
	a_moduleinfo's module_version()
end module_version_of

on try_collect(mspec, adpaths)
	set a_record to _load module_ mspec additional paths adpaths
	set a_source to file of a_record
	set a_script to script of a_record
	set a_location to item 1 of adpaths
	tell application "Finder"
		set src_name to name of a_source
		try
			set new_alias to make alias file at a_location to a_source with properties {name:src_name}
		on error msg number errno
			error msg & return & "Failed to make an alias file of " & (quoted form of name of mspec) number errno
		end try
	end tell
	return a_record
end try_collect

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

on clear_cache()
	set my _module_cache to make ModuleCache
	return me
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
	--boot (module loader) for me
	boot for me
end run
*)