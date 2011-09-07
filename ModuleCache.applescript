global FastList
property XList : missing value

on run
	return me
end run

on make
	return make_with_lists({}, {}, {})
end make

on initialize()
	set XList to FastList
	return me
end initialize

on make_with_lists(name_list, path_list, value_list)
	set name_list to XList's make_with(name_list)
	set path_list to XList's make_with(path_list)
	set value_list to XList's make_with(value_list)
	return make_with_xlists(name_list, path_list, value_list)
end make_with_lists

on make_with_xlists(name_list, path_list, value_list)
	set a_parent to me
	script ModuleCache
		property parent : a_parent
		property _names : name_list
		property _paths : path_list
		property _values : value_list
	end script
	return ModuleCache
end make_with_xlists

on search_value(a_key, key_list, value_list)
	-- when a value is a record, "is in" operator does not works.
	set an_index to key_list's index_of(a_key)
	if an_index is 0 then
		error number 900
	end if
	
	return value_list's item_at(an_index)
end search_value

on module_for_name(a_name)
	return search_value(a_name, my _names, my _values)
end module_for_name

on module_for_path(a_path)
	return search_value(a_path, my _paths, my _values)
end module_for_path

on module_for_specifier(mspec)
	set an_index to 0
	set required_name to mspec's name
	try
		set required_version to mspec's version
	on error
		return module_for_name(required_name)
	end try
	repeat with n from 1 to (my _values's count_items())
		set a_name to my _names's item_at(n)
		set a_moduleinfo to my _values's item_at(n)
		if a_name is required_name then
			set a_version to a_moduleinfo's module_version()
			if meet the version a_version condition required_version then
				return a_moduleinfo
			end if
		end if
	end repeat
	
	error number 900
end module_for_specifier

on module_for_name_version(required_name, required_version)
	set an_index to 0
	repeat with n from 1 to (my _values's count_items())
		set a_name to my _names's item_at(n)
		set a_moduleinfo to my _values's item_at(n)
		if a_name is required_name then
			set a_version to a_moduleinfo's module_version()
			if (a_version is not missing value) ¬
				and (meet the version a_version condition required_version) then
				return a_moduleinfo
			end if
		end if
	end repeat
	
	error number 900
end module_for_name_version

on module_for_script(a_script)
	set an_index to 0
	repeat with n from 1 to (my _values's count_items())
		set a_moduleinfo to my _values's item_at(n)
		if a_script is a_moduleinfo's module_script() then
			return a_moduleinfo
		end if
	end repeat
	
	error number 900
end module_for_script

on replace_module(a_name, a_path, a_moduleinfo)
	set an_index to my _names's index_of(a_name)
	if an_index is 0 then
		add_module(a_name, a_path, a_moduleinfo)
		return
	end if
	set_item of (my _names) for a_name at an_index
	set_item of (my _paths) for a_path at an_index
	set_item of (my _values) for a_moduleinfo at an_index
end replace_module

on add_module(a_name, a_path, a_moduleinfo)
	my _names's push(a_name)
	my _paths's push(a_path)
	my _values's push(a_moduleinfo)
end add_module

on prepend_module(a_name, a_path, a_moduleinfo)
	my _names's unshift(a_name)
	my _paths's unshift(a_path)
	my _values's unshift(a_moduleinfo)
end prepend_module