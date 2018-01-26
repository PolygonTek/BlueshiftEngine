local blueshift = require 'blueshift'
local json = require 'dkjson'
local vscode_debugee = require 'vscode-debuggee'
local startResult = false
local breakerType = ''

function start(debugger_server_addr)
	local config = { redirectPrint = true, controllerHost = debugger_server_addr }
	startResult, breakerType = debuggee.start(json, config)
	if startResult then
	    blueshift.log('Successfully connected to debugger ('..breakerType..'): '..debugger_server_addr)
	else
	    blueshift.log('Failed to connect to debugger: '..debugger_server_addr)
	end
end()

function stop()
end

function poll()
	if startResult then
		vscode_debugee.poll()
	end
end