property _recordLog : false
property _sender : ""

on run
	return me
end run

on set_name(a_name)
	set __name__ to a_name
end set_name

on start_log()
	set my _recordLog to true
	return me
end start_log

on stop_log()
	set my _recordLog to false
	return me
end stop_log

on do(a_message)
	if my _recordLog then
		--set a_message to ((current date) as Unicode text) & space & my _sender & " : " & a_message
		--do shell script "syslog -l 5 -s " & quoted form of a_message
		do shell script "logger -p user.warning  -t " & my _sender & " -s " & quoted form of a_message
	end if
end do

on make_with(sender_name)
	set a_parent to me
	script ConsoleLog
		property parent : a_parent
		property _recordLog : true
		property _sender : sender_name
	end script
	return ConsoleLog
end make_with