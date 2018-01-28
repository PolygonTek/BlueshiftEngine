local blueshift = require 'blueshift'
local mobdebug = require 'mobdebug'
local startResult = false

function start(debugger_server_addr)
	mobdebug.onexit = stop
	startResult = mobdebug.start(debugger_server_addr)
	if startResult then
    	blueshift.log('Successfully connected to debugger: '..debugger_server_addr)
	else
    	blueshift.log('Failed to connect to debugger: '..debugger_server_addr)
	end
end

function stop()
	if startResult then
		mobdebug.done()
		blueshift.log('Stop debugger')
	end
end