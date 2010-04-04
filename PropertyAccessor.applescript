global FastList
property XList : missing value

property _keys : missing value
property _accessors : missing value

on run
	return me
end run

on initialize()
	set XList to FastList
	set my _keys to make XList
	set my _accessors to make XList
	return me
end initialize

on make_with_name(a_name)
	set ind to my _keys's index_of(a_name)
	if ind is not 0 then
		--log "hit in accessr cache"
		return my _accessors's item_at(ind)
	end if
	
	set an_accessor to run script "
on set_value(an_object, a_value)
set " & a_name & " of an_object to a_value
return an_object
end set_value
return me"
	my _keys's push(a_name)
	my _accessors's push(an_accessor)
	return an_accessor
end make_with_name
