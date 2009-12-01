property name : "LocalLoader"
property _idleTime : 0
property _idleInterval : 60 * 5
property _waitTime : _idleInterval
property _loaderCore : missing value
property _only_local : false

on setup()
	--display dialog (path to me as text)
	set my _loaderCore to continue make loader
	my _loaderCore's set_local(true)
end setup

on load(a_name)
	return load_module(a_name)
end load

on load module a_name
	return load_module(a_name)
end load module

on load_module(a_name)
	set a_loader to make_loader()
	return a_loader's load_module(a_name)
end load_module

on module loader
	return make_loader()
end module loader

on proxy()
	return make_loader()
end proxy

on make_loader()
	copy my _loaderCore to local_loader
	local_loader's set_local(true)
	local_loader's set_localonly(my _only_local)
	local_loader's set_additional_paths({local_loader's current_location()})
	return local_loader
end make_loader

on makeLoader() -- AppleMods
	return make_loader()
end makeLoader

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