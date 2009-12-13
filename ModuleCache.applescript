global XList

on run
	return me
end run

on make
	return make_with_lists({}, {}, {})
end make

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

on add_module(a_name, a_path, a_moduleinfo)
	my _names's push(a_name)
	my _paths's push(a_path)
	my _values's push(a_moduleinfo)
end add_module