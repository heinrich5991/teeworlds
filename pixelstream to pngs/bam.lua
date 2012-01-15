CheckVersion("0.4")

function Intermediate_Output(settings, input)
	return "objs/" .. string.sub(PathBase(input), string.len("src/")+1) .. settings.config_ext
end

function build(settings)
	-- set some platform specific settings
	settings.cc.includes:Add("src")

	-- compile zlib if needed
    zlib = Compile(settings, Collect("src/zlib/*.c"))
    settings.cc.includes:Add("src/zlib")

	-- build the small libraries
	pnglite = Compile(settings, Collect("src/pnglite/*.c"))


	srccompile = Compile(settings, Collect("src/*.cpp"), Collect("src/base/*.c"))

	if family == "unix" then
   		if platform == "macosx" then
			settings.link.frameworks:Add("Carbon")
			settings.link.frameworks:Add("AppKit")
		else
			settings.link.libs:Add("pthread")
		end
	elseif family == "windows" then
		settings.link.libs:Add("gdi32")
		settings.link.libs:Add("user32")
		settings.link.libs:Add("ws2_32")
		settings.link.libs:Add("ole32")
		settings.link.libs:Add("shell32")
	end

	-- build client, server, version server and master server
	exe = Link(settings, "pixelstream2png", srccompile, zlib, pnglite)


	-- make targets
	c = PseudoTarget("client".."_"..settings.config_name, exe)

	all = PseudoTarget(settings.config_name, c)

	return all
end


debug_settings = NewSettings()
debug_settings.config_name = "debug"
debug_settings.config_ext = "_d"
debug_settings.debug = 1
debug_settings.optimize = 0
debug_settings.cc.defines:Add("CONF_DEBUG")

release_settings = NewSettings()
release_settings.config_name = "release"
release_settings.config_ext = ""
release_settings.debug = 0
release_settings.optimize = 1
release_settings.cc.defines:Add("CONF_RELEASE")
release_settings.cc.flags:Add("/Ob2xt")

if platform == "macosx"  and arch == "ia32" then
	debug_settings_ppc = debug_settings:Copy()
	debug_settings_ppc.config_name = "debug_ppc"
	debug_settings_ppc.config_ext = "_ppc_d"
	debug_settings_ppc.cc.flags:Add("-arch ppc")
	debug_settings_ppc.link.flags:Add("-arch ppc")
	debug_settings_ppc.cc.defines:Add("CONF_DEBUG")

	release_settings_ppc = release_settings:Copy()
	release_settings_ppc.config_name = "release_ppc"
	release_settings_ppc.config_ext = "_ppc"
	release_settings_ppc.cc.flags:Add("-arch ppc")
	release_settings_ppc.link.flags:Add("-arch ppc")
	release_settings_ppc.cc.defines:Add("CONF_RELEASE")

	debug_settings_x86 = debug_settings:Copy()
	debug_settings_x86.config_name = "debug_x86"
	debug_settings_x86.config_ext = "_x86_d"
	debug_settings_x86.cc.flags:Add("-arch i386")
	debug_settings_x86.link.flags:Add("-arch i386")
	debug_settings_x86.cc.defines:Add("CONF_DEBUG")

	release_settings_x86 = release_settings:Copy()
	release_settings_x86.config_name = "release_x86"
	release_settings_x86.config_ext = "_x86"
	release_settings_x86.cc.flags:Add("-arch i386")
	release_settings_x86.link.flags:Add("-arch i386")
	release_settings_x86.cc.defines:Add("CONF_RELEASE")

	ppc_d = build(debug_settings_ppc)
	x86_d = build(debug_settings_x86)
	ppc_r = build(release_settings_ppc)
	x86_r = build(release_settings_x86)
	DefaultTarget("game_debug_x86")
	PseudoTarget("release", ppc_r, x86_r)
	PseudoTarget("debug", ppc_d, x86_d)

	PseudoTarget("server_release", "server_release_x86", "server_release_ppc")
	PseudoTarget("server_debug", "server_debug_x86", "server_debug_ppc")
	PseudoTarget("client_release", "client_release_x86", "client_release_ppc")
	PseudoTarget("client_debug", "client_debug_x86", "client_debug_ppc")
else
	build(debug_settings)
	build(release_settings)
end
