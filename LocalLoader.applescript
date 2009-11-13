property _idleTime : 0
property _idleInterval : 60 * 5
property _waitTime : _idleInterval
property _loaderCore : missing value

on setup()
	set my _loaderCore to continue make loader
	my _loaderCore's set_local(true)
end setup

on load(a_name)
	set a_loader to proxy()
	return a_loader's load(a_name)
end load

on make loader
	return proxy()
end make loader

on proxy()
	copy my _loaderCore to local_loader
	local_loader's set_local(true)
	local_loader's set_additional_paths(current_location())
	return local_loader
end proxy

on idle
	--ConsoleLog's do("start idle")
	if _idleTime is greater than or equal to _waitTime then
		--ConsoleLog's do("will quit")
		quit
		return 1
	end if
	set _idleTime to _idleTime + _idleInterval
	return _idleInterval
end idle

on quit
	--ConsoleLog's do("start quit")
	continue quit
	--ConsoleLog's do("did quit")
end quit

on run
	--return debug()
	set my _idleTime to 0
end run