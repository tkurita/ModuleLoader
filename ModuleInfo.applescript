on set_setupped(bool)
	set my _setupped to bool
end set_setupped

on is_setupped()
	return my _setupped
end is_setupped

on need_setup()
	return not my _setupped
end need_setup

on dependencies()
	return my _dependecies
end dependencies

on module_script()
	return my _script
end module_script

on make_with_loadinfo(loadinfo)
	return make_with_vars(script of loadinfo, dependency info of loadinfo, false)
end make_with_loadinfo

on make_with_vars(a_script, dependencies_list, setuppped_flag)
	script ModuleInfo
		property _script : a_script
		property _dependecies : dependencies_list
		property _setupped : setuppped_flag
	end script
end make_with_vars

return me