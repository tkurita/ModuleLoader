property name : "FastList"
property version : "1.1"

on run
	return me
end run

on make
	return make_with({})
end make

on make_with(a_list)
	set a_parent to me
	script FastList
		property parent : a_parent
		property _contents : a_list
		property _length : length of _contents
		property _n : 1
		property _currentItem : missing value
	end script
	
	return FastList
end make_with

on next()
	try
		set an_item to item (my _n) of my _contents
	on error msg number -1728
		error "No next item." number 1351
	end try
	
	set my _n to (my _n) + 1
	return an_item
end next

on next_item()
	return next()
end next_item

on has_next()
	return my _n is less than or equal to my _length
end has_next

on current_item()
	return item ((my _n) - 1) of my _contents
end current_item

on current_index()
	return (my _n) - 1
end current_index

on decrement_index()
	if my _n > 1 then
		set my _n to (my _n) - 1
	end if
end decrement_index

on increment_index()
	if my _n > 1 then
		set my _n to (my _n) + 1
	end if
end increment_index

(*!@group Stack and Quene *)

on push(an_item)
	set end of my _contents to an_item
	set my _length to (my _length) + 1
end push

on unshift(a_item)
	set beginning of my _contents to an_item
	set my _length to (my _length) + 1
end unshift


(*!@group Accessing List Items *)
on count_items()
	return count my _contents
end count_items

on count
	return continue count my _contents
end count

on delete_at(indexes)
	set indexes to indexes as list
	set a_list to {}
	
	repeat with n from 1 to length of indexes
		set an_index to item n of indexes
		--log "start delete_item"
		set end of a_list to item an_index of my _contents
		if an_index is 1 then
			set my _contents to rest of my _contents
		else if an_index is in {my _length, -1} then
			set my _contents to items 1 thru -2 of my _contents
		else
			set my _contents to (items 1 thru (an_index - 1) of my _contents) & (items (an_index + 1) thru -1 of my _contents)
		end if
		
		if (my _n > an_index) then
			set my _n to (my _n) + 1
		end if
		
		set my _length to (my _length) - 1
	end repeat
	--log "end delete_item"
	return a_list
end delete_at

on item_at(an_index)
	return item an_index of my _contents
end item_at

on set_item for a_value at an_index
	set item an_index of my _contents to a_value
end set_item

on has_item(an_item)
	return an_item is in my _contents
end has_item

on index_of(an_item)
	if not has_item(an_item) then
		return 0
	end if
	
	set an_index to 0
	repeat with n from 1 to my _length
		if item n of my _contents is an_item then
			set an_index to n
			exit repeat
		end if
	end repeat
	
	return an_index
end index_of




