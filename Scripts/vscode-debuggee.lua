local debuggee = {}

local socket = require 'socket.core'
local json
local handlers = {}
local sock
local directorySeperator = '\\'
local sourceBasePath = '.'
local storedVariables = {}
local nextVarRef = 1
local baseDepth
local breaker
local sendEvent
local dumpCommunication = false
local ignoreFirstFrameInC = false
local debugTargetCo = nil
local redirectedPrintFunction = nil

local onError = nil

local function defaultOnError(e)
	print('****************************************************')
	print(e)
	print('****************************************************')
end

local function valueToString(value, depth)
	local str = ''
	depth = depth or 0
	local t = type(value)
	if t == 'table' then
		str = str .. '{\n'
		for k, v in pairs(value) do
			str = str .. string.rep('  ', depth + 1) .. '[' .. valueToString(k) ..']' .. ' = ' .. valueToString(v, depth + 1) .. ',\n'
		end
		str = str .. string.rep('  ', depth) .. '}'
	elseif t == 'string' then
		str = str .. '"' .. tostring(value) .. '"'
	else
		str = str .. tostring(value)
	end
	return str
end

-------------------------------------------------------------------------------
local sethook = debug.sethook
debug.sethook = nil

local cocreate = coroutine.create
coroutine.create = function(f)
	local c = cocreate(f)
	debuggee.addCoroutine(c)
	return c
end

-------------------------------------------------------------------------------
local function debug_getinfo(depth, what)
	if debugTargetCo then
		return debug.getinfo(debugTargetCo, depth, what)
	else
		return debug.getinfo(depth + 1, what)
	end
end

-------------------------------------------------------------------------------
local function debug_getlocal(depth, i)
	if debugTargetCo then
		return debug.getlocal(debugTargetCo, depth, i)
	else
		return debug.getlocal(depth + 1, i)
	end
end

-------------------------------------------------------------------------------
local DO_TEST = false

-------------------------------------------------------------------------------
-- chunkname 매칭 {{{
local function getMatchCount(a, b)
	local n = math.min(#a, #b)
	for i = 0, n - 1 do
		if a[#a - i] == b[#b - i] then
			-- pass
		else
			return i
		end
	end
	return n
end
if DO_TEST then
	assert(getMatchCount({'a','b','c'}, {'a','b','c'}) == 3)
	assert(getMatchCount({'b','c'}, {'a','b','c'}) == 2)
	assert(getMatchCount({'a','b','c'}, {'b','c'}) == 2)
	assert(getMatchCount({}, {'a','b','c'}) == 0)
	assert(getMatchCount({'a','b','c'}, {}) == 0)
	assert(getMatchCount({'a','b','c'}, {'a','b','c','d'}) == 0)
end

local function splitChunkName(s)
	if string.sub(s, 1, 1) == '@' then
		s = string.sub(s, 2)
	end

	local a = {}
	for word in string.gmatch(s, '[^/\\]+') do
		a[#a + 1] = string.lower(word)
	end
	return a
end
if DO_TEST then
	local a = splitChunkName('@.\\vscode-debuggee.lua')
	assert(#a == 2)
	assert(a[1] == '.')
	assert(a[2] == 'vscode-debuggee.lua')

	local a = splitChunkName('@C:\\dev\\VSCodeLuaDebug\\debuggee/lua\\socket.lua')
	assert(#a == 6)
	assert(a[1] == 'c:')
	assert(a[2] == 'dev')
	assert(a[3] == 'vscodeluadebug')
	assert(a[4] == 'debuggee')
	assert(a[5] == 'lua')
	assert(a[6] == 'socket.lua')

	local a = splitChunkName('@main.lua')
	assert(#a == 1)
	assert(a[1] == 'main.lua')
end
-- chunkname 매칭 }}}

-- 패스 조작 {{{
local Path = {}

function Path.isAbsolute(a)
	local firstChar = string.sub(a, 1, 1)
	if firstChar == '/' or
	   firstChar == '\\' then
		return true
	end

	if string.match(a, '^%a%:[/\\]') then
		return true
	end

	return false
end

function Path.concat(a, b)
	-- a를 노멀라이즈
	local lastChar = string.sub(a, #a, #a)
	if (lastChar == '/' or lastChar == '\\') then
		-- pass
	else
		a = a .. directorySeperator
	end

	-- b를 노멀라이즈
	if string.match(b, '^%.%\\') then
		b = string.sub(b, 3)
	end

	return a .. b
end

function Path.toAbsolute(base, sub)
	if Path.isAbsolute(sub) then
		return sub
	else
		return Path.concat(base, sub)
	end
end

if DO_TEST then
	assert(Path.isAbsolute('c:\\asdf\\afsd'))
	assert(Path.isAbsolute('c:/asdf/afsd'))
	assert(Path.concat('c:\\asdf', 'fdsf') == 'c:\\asdf\\fdsf')
	assert(Path.concat('c:\\asdf', '.\\fdsf') == 'c:\\asdf\\fdsf')
end
-- 패스 조작 }}}

local coroutineSet = {}
setmetatable(coroutineSet, { __mode = 'v' })

-------------------------------------------------------------------------------
-- 네트워크 유틸리티 {{{
local function sendFully(str)
	local first = 1
	while first <= #str do
		local sent = sock:send(str, first)
		if sent and sent > 0 then
			first = first + sent;
		else
			error('sock:send() returned < 0')
		end
	end
end

-- send log to debug console
local function logToDebugConsole(output, category)
	local dumpMsg = {
		event = 'output',
		type = 'event',
		body = {
			category = category or 'console',
			output = output
		}
	}
	local dumpBody = json.encode(dumpMsg)
	sendFully('#' .. #dumpBody .. '\n' .. dumpBody)
end

-- 순정 모드 {{{
local function createHaltBreaker()
	-- chunkname 매칭 {
	local loadedChunkNameMap = {}
	for chunkname, _ in pairs(debug.getchunknames()) do
		loadedChunkNameMap[chunkname] = splitChunkName(chunkname)
	end

	local function findMostSimilarChunkName(path)
		local splitedReqPath = splitChunkName(path)
		local maxMatchCount = 0
		local foundChunkName = nil
		for chunkName, splitted in pairs(loadedChunkNameMap) do
			local count = getMatchCount(splitedReqPath, splitted)
			if (count > maxMatchCount) then
				maxMatchCount = count
				foundChunkName = chunkName
			end
		end
		return foundChunkName
	end
	-- chunkname 매칭 }

	local lineBreakCallback = nil
	local function updateCoroutineHook(c)
		if lineBreakCallback then
			sethook(c, lineBreakCallback, 'l')
		else
			sethook(c)
		end
	end
	local function sethalt(cname, ln)
		for i = ln, ln + 10 do
			if debug.sethalt(cname, i) then
				return i
			end
		end
		return nil
	end
	return {
		setBreakpoints = function(path, lines)
			local foundChunkName = findMostSimilarChunkName(path)
			local verifiedLines = {}

			if foundChunkName then
				debug.clearhalt(foundChunkName)
				for _, ln in ipairs(lines) do
					verifiedLines[ln] = sethalt(foundChunkName, ln)
				end
			end

			return verifiedLines
		end,

		setLineBreak = function(callback)
			if callback then
				sethook(callback, 'l')
			else
				sethook()
			end

			lineBreakCallback = callback
			for cid, c in pairs(coroutineSet) do
				updateCoroutineHook(c)
			end
		end,

		coroutineAdded = function(c)
			updateCoroutineHook(c)
		end,

		-- 실험적으로 알아낸 값들-_-ㅅㅂ
		stackOffset =
		{
			enterDebugLoop = 6,
			halt = 6,
			step = 4,
			stepDebugLoop = 6
		}
	}
end

local function createPureBreaker()
	local lineBreakCallback = nil
	local breakpointsPerPath = {}
	local chunknameToPathCache = {}

	local function chunkNameToPath(chunkname)
		local cached = chunknameToPathCache[chunkname]
		if cached then
			return cached
		end

		local splitedReqPath = splitChunkName(chunkname)
		local maxMatchCount = 0
		local foundPath = nil
		for path, _ in pairs(breakpointsPerPath) do
			local splitted = splitChunkName(path)
			local count = getMatchCount(splitedReqPath, splitted)
			if (count > maxMatchCount) then
				maxMatchCount = count
				foundPath = path
			end
		end

		if foundPath then
			chunknameToPathCache[chunkname] = foundPath
		end
		return foundPath
	end

	local entered = false
	local function hookfunc()
		if entered then return false end
		entered = true

		if lineBreakCallback then
			lineBreakCallback()
		end

		local info = debug_getinfo(2, 'Sl')
		if info then
			local path = chunkNameToPath(info.source)
			if path then
				path = string.lower(path)
			end
			local bpSet = breakpointsPerPath[path]
			if bpSet and bpSet[info.currentline] then
				_G.__halt__()
			end
		end

		entered = false
	end
	sethook(hookfunc, 'l')

	return {
		setBreakpoints = function(path, lines)
			local t = {}
			local verifiedLines = {}
			for _, ln in ipairs(lines) do
				t[ln] = true
				verifiedLines[ln] = ln
			end
			if path then
				path = string.lower(path)
			end
			breakpointsPerPath[path] = t
			return verifiedLines
		end,

		setLineBreak = function(callback)
			lineBreakCallback = callback
		end,

		coroutineAdded = function(c)
			sethook(c, hookfunc, 'l')
		end,

		-- 실험적으로 알아낸 값들-_-ㅅㅂ
		stackOffset =
		{
			enterDebugLoop = 6,
			halt = 7,
			step = 4,
			stepDebugLoop = 7
		}
	}
end

-- 순정 모드 }}}


-- 센드는 블럭이어도 됨.
local function sendMessage(msg)
	local body = json.encode(msg)

	if dumpCommunication then
		logToDebugConsole('[SENDING] ' .. valueToString(msg))
	end

	sendFully('#' .. #body .. '\n' .. body)
end

-- 리시브는 블럭이 아니어야 할 거 같은데... 음... 블럭이어도 괜찮나?
local function recvMessage()
	local header = sock:receive('*l')
	if (header == nil) then
		-- 디버거가 떨어진 상황
		return nil
	end
	if (string.sub(header, 1, 1) ~= '#') then
		error('헤더 이상함:' .. header)
	end

	local bodySize = tonumber(header:sub(2))
	local body = sock:receive(bodySize)

	return json.decode(body)
end
-- 네트워크 유틸리티 }}}

-------------------------------------------------------------------------------
local function debugLoop()
	storedVariables = {}
	nextVarRef = 1
	while true do
		local msg = recvMessage()
		if msg then
			if dumpCommunication then
				logToDebugConsole('[RECEIVED] ' .. valueToString(msg), 'stderr')
			end

			local fn = handlers[msg.command]
			if fn then
				local rv = fn(msg)

				-- continue인데 break하는 게 역설적으로 느껴지지만
				-- 디버그 루프를 탈출(break)해야 정상 실행 흐름을 계속(continue)할 수 있지..
				if (rv == 'CONTINUE') then
					break;
				end
			else
				--print('UNKNOWN DEBUG COMMAND: ' .. tostring(msg.command))
			end
		else
			-- 디버그 중에 디버거가 떨어졌다.
			-- print펑션을 리다이렉트 한경우에는 원래대로 돌려놓는다
			if redirectedPrintFunction then
				_G.print = redirectedPrintFunction
			end
			break
		end
	end
	storedVariables = {}
	nextVarRef = 1
end

-------------------------------------------------------------------------------
local sockArray = {}
function debuggee.start(jsonLib, config)
	json = jsonLib
	assert(jsonLib)

	config = config or {}
	local connectTimeout = config.connectTimeout or 5.0
	local controllerHost = config.controllerHost or 'localhost'
	local controllerPort = config.controllerPort or 56789
	onError              = config.onError or defaultOnError
	local redirectPrint  = config.redirectPrint or false
	dumpCommunication    = config.dumpCommunication or false
	ignoreFirstFrameInC  = config.ignoreFirstFrameInC or false
	if not config.luaStyleLog then
		valueToString = function(value) return json.encode(value) end
	end

	local breakerType
	if debug.sethalt then
		breaker = createHaltBreaker()
		breakerType = 'halt'
	else
		breaker = createPureBreaker()
		breakerType = 'pure'
	end

	local err
	sock, err = socket.tcp()
	if not sock then error(err) end
	sockArray = { sock }
	if sock.settimeout then sock:settimeout(connectTimeout) end
	local res, err = sock:connect(controllerHost, tostring(controllerPort))
	if not res then
		sock:close()
		sock = nil
		return false, breakerType
	end

	if sock.settimeout then sock:settimeout() end
	sock:setoption('tcp-nodelay', true)

	local initMessage = recvMessage()
	assert(initMessage and initMessage.command == 'welcome')
	sourceBasePath = initMessage.sourceBasePath
	directorySeperator = initMessage.directorySeperator

	if redirectPrint then
		redirectedPrintFunction = _G.print -- 디버거가 떨어질때를 대비해서 보관한다
		_G.print = function(...)
			local t = { n = select("#", ...), ... }
			for i = 1, #t do
				t[i] = tostring(t[i])
			end
			sendEvent(
				'output',
				{
					category = 'stdout',
					output = table.concat(t, '\t') .. '\n' -- Same as default "print" output end new line.
				})
		end
	end

	debugLoop()
	return true, breakerType
end

-------------------------------------------------------------------------------
function debuggee.poll()
	if not sock then return end

	-- Processes commands in the queue.
	-- Immediately returns when the queue is/became empty.
	while true do
		local r, w, e = socket.select(sockArray, nil, 0)
		if e == 'timeout' then break end

		local msg = recvMessage()
		if msg then
			if dumpCommunication then
				logToDebugConsole('[POLL-RECEIVED] ' .. valueToString(msg), 'stderr')
			end

			if msg.command == 'pause' then
				debuggee.enterDebugLoop(1)
				return
			end

			local fn = handlers[msg.command]
			if fn then
				local rv = fn(msg)
				-- Ignores rv, because this loop never blocks except explicit pause command.
			else
				--print('POLL-UNKNOWN DEBUG COMMAND: ' .. tostring(msg.command))
			end
		else
			break
		end
	end
end

-------------------------------------------------------------------------------
local function getCoroutineId(c)
	-- 'thread: 011DD5B0'
	--  12345678^
	local threadIdHex = string.sub(tostring(c), 9)
	return tonumber(threadIdHex, 16)
end

-------------------------------------------------------------------------------
function debuggee.addCoroutine(c)
	local cid = getCoroutineId(c)
	coroutineSet[cid] = c
	breaker.coroutineAdded(c)
end

-------------------------------------------------------------------------------
local function sendSuccess(req, body)
	sendMessage({
		command = req.command,
		success = true,
		request_seq = req.seq,
		type = "response",
		body = body
	})
end

-------------------------------------------------------------------------------
local function sendFailure(req, msg)
	sendMessage({
		command = req.command,
		success = false,
		request_seq = req.seq,
		type = "response",
		message = msg
	})
end

-------------------------------------------------------------------------------
sendEvent = function(eventName, body)
	sendMessage({
		event = eventName,
		type = "event",
		body = body
	})
end

-------------------------------------------------------------------------------
local function currentThreadId()
--[[
	local threadId = 0
	if coroutine.running() then
	end
	return threadId
]]
	return 0
end

-------------------------------------------------------------------------------
local function startDebugLoop()
	sendEvent(
		'stopped',
		{
			reason = 'breakpoint',
			threadId = currentThreadId(),
			allThreadsStopped = true
		})

	local status, err = pcall(debugLoop)
	if not status then
		onError(err)
	end
end

-------------------------------------------------------------------------------
_G.__halt__ = function()
	baseDepth = breaker.stackOffset.halt
	startDebugLoop()
end

-------------------------------------------------------------------------------
function debuggee.enterDebugLoop(depthOrCo, what)
	if sock == nil then
		return false
	end

	if what then
		sendEvent(
			'output',
			{
				category = 'stderr',
				output = what,
			})
	end

	if type(depthOrCo) == 'thread' then
		baseDepth = 0
		debugTargetCo = depthOrCo
	elseif type(depthOrCo) == 'table' then
		baseDepth = (depthOrCo.depth or 0)
		debugTargetCo = depthOrCo.co
	else
		baseDepth = (depthOrCo or 0) + breaker.stackOffset.enterDebugLoop
		debugTargetCo = nil
	end
	startDebugLoop()
	return true
end

-------------------------------------------------------------------------------
-- ★★★ https://github.com/Microsoft/vscode/blob/master/src/vs/workbench/parts/debug/common/debugProtocol.d.ts
-------------------------------------------------------------------------------

-------------------------------------------------------------------------------
function handlers.setBreakpoints(req)
	local bpLines = {}
	for _, bp in ipairs(req.arguments.breakpoints) do
		bpLines[#bpLines + 1] = bp.line
	end

	local verifiedLines = breaker.setBreakpoints(
		req.arguments.source.path,
		bpLines)

	local breakpoints = {}
	for i, ln in ipairs(bpLines) do
		breakpoints[i] = {
			verified = (verifiedLines[ln] ~= nil),
			line = verifiedLines[ln]
		}
	end

	sendSuccess(req, {
		breakpoints = breakpoints
	})
end

-------------------------------------------------------------------------------
function handlers.configurationDone(req)
	sendSuccess(req, {})
	return 'CONTINUE'
end

-------------------------------------------------------------------------------
function handlers.threads(req)
	local c = coroutine.running()

	local mainThread = {
		id = currentThreadId(),
		name = (c and tostring(c)) or "main"
	}

	sendSuccess(req, {
		threads = { mainThread }
	})
end

-------------------------------------------------------------------------------
function handlers.stackTrace(req)
	assert(req.arguments.threadId == 0)

	local stackFrames = {}
	local firstFrame = (req.arguments.startFrame or 0) + baseDepth
	local lastFrame = (req.arguments.levels and (req.arguments.levels ~= 0))
		and (firstFrame + req.arguments.levels - 1)
		or (9999)

	-- if firstframe function of stack is C function, ignore it.
	if ignoreFirstFrameInC then
		local info = debug_getinfo(firstFrame, 'lnS')
		if info and info.what == "C" then
			firstFrame = firstFrame + 1
		end
	end

	for i = firstFrame, lastFrame do
		local info = debug_getinfo(i, 'lnS')
		if (info == nil) then break end
		--print(json.encode(info))

		local src = info.source
		if string.sub(src, 1, 1) == '@' then
			src = string.sub(src, 2) -- 앞의 '@' 떼어내기
		end

		local name
		if info.name then
			name = info.name .. ' (' .. (info.namewhat or '?') .. ')'
		else
			name = '?'
		end

		local sframe = {
			name = name,
			source = {
				name = nil,
				path = Path.toAbsolute(sourceBasePath, src)
			},
			column = 1,
			line = info.currentline or 1,
			id = i,
		}
		stackFrames[#stackFrames + 1] = sframe
	end

	sendSuccess(req, {
		stackFrames = stackFrames
	})
end

-------------------------------------------------------------------------------
local scopeTypes = {
	Locals = 1,
	Upvalues = 2,
	Globals = 3,
}
function handlers.scopes(req)
	local depth = req.arguments.frameId

	local scopes = {}
	local function addScope(name)
		scopes[#scopes + 1] = {
			name = name,
			expensive = false,
			variablesReference = depth * 1000000 + scopeTypes[name]
		}
	end

	addScope('Locals')
	addScope('Upvalues')
	addScope('Globals')

	sendSuccess(req, {
		scopes = scopes
	})
end

-------------------------------------------------------------------------------
local function registerVar(varNameCount, name_, value, noQuote)
	local ty = type(value)
	local name
	if type(name_) == 'number' then
		name = '[' .. name_ .. ']'
	else
		name = tostring(name_)
	end
	if varNameCount[name] then
		varNameCount[name] = varNameCount[name] + 1
		name = name .. ' (' .. varNameCount[name] .. ')'
	else
		varNameCount[name] = 1
	end

	local item = {
		name = name,
		type = ty
	}

	if (ty == 'string' and (not noQuote)) then
		item.value = '"' .. value .. '"'
	else
		item.value = tostring(value)
	end

	if (ty == 'table') or
		(ty == 'function') then
		storedVariables[nextVarRef] = value
		item.variablesReference = nextVarRef
		nextVarRef = nextVarRef + 1
	else
		item.variablesReference = -1
	end

	return item
end

-------------------------------------------------------------------------------
function handlers.variables(req)
	local varRef = req.arguments.variablesReference
	local variables = {}
	local varNameCount = {}
	local function addVar(name, value, noQuote)
		variables[#variables + 1] = registerVar(varNameCount, name, value, noQuote)
	end

	if (varRef >= 1000000) then
		-- Scope.
		local depth = math.floor(varRef / 1000000)
		local scopeType = varRef % 1000000
		if scopeType == scopeTypes.Locals then
			for i = 1, 9999 do
				local name, value = debug_getlocal(depth, i)
				if name == nil then break end
				addVar(name, value, nil)
			end
		elseif scopeType == scopeTypes.Upvalues then
			local info = debug_getinfo(depth, 'f')
			if info and info.func then
				for i = 1, 9999 do
					local name, value = debug.getupvalue(info.func, i)
					if name == nil then break end
					addVar(name, value, nil)
				end
			end
		elseif scopeType == scopeTypes.Globals then
			for name, value in pairs(_G) do
				addVar(name, value)
			end
			table.sort(variables, function(a, b) return a.name < b.name end)
		end
	else
		-- Expansion.
		local var = storedVariables[varRef]
		if type(var) == 'table' then
			for k, v in pairs(var) do
				addVar(k, v)
			end
			table.sort(variables, function(a, b)
				local aNum, aMatched = string.gsub(a.name, '^%[(%d+)%]$', '%1')
				local bNum, bMatched = string.gsub(b.name, '^%[(%d+)%]$', '%1')

				if (aMatched == 1) and (bMatched == 1) then
					-- both are numbers. compare numerically.
					return tonumber(aNum) < tonumber(bNum)
				elseif aMatched == bMatched then
					-- both are strings. compare alphabetically.
					return a.name < b.name
				else
					-- string comes first.
					return aMatched < bMatched
				end
			end)
		elseif type(var) == 'function' then
			local info = debug.getinfo(var, 'S')
			addVar('(source)', tostring(info.short_src), true)
			addVar('(line)', info.linedefined)

			for i = 1, 9999 do
				local name, value = debug.getupvalue(var, i)
				if name == nil then break end
				addVar(name, value)
			end
		end

		local mt = getmetatable(var)
		if mt then
			addVar("(metatable)", mt)
		end
	end

	sendSuccess(req, {
		variables = variables
	})
end

-------------------------------------------------------------------------------
function handlers.continue(req)
	sendSuccess(req, {})
	return 'CONTINUE'
end

-------------------------------------------------------------------------------
local function stackHeight()
	for i = 1, 9999999 do
		if (debug_getinfo(i, '') == nil) then
			return i
		end
	end
end

-------------------------------------------------------------------------------
local stepTargetHeight = nil
local function step()
	if (stepTargetHeight == nil) or (stackHeight() <= stepTargetHeight) then
		breaker.setLineBreak(nil)
		baseDepth = breaker.stackOffset.stepDebugLoop
		startDebugLoop()
	end
end

-------------------------------------------------------------------------------
function handlers.next(req)
	stepTargetHeight = stackHeight() - breaker.stackOffset.step
	breaker.setLineBreak(step)
	sendSuccess(req, {})
	return 'CONTINUE'
end

-------------------------------------------------------------------------------
function handlers.stepIn(req)
	stepTargetHeight = nil
	breaker.setLineBreak(step)
	sendSuccess(req, {})
	return 'CONTINUE'
end

-------------------------------------------------------------------------------
function handlers.stepOut(req)
	stepTargetHeight = stackHeight() - (breaker.stackOffset.step + 1)
	breaker.setLineBreak(step)
	sendSuccess(req, {})
	return 'CONTINUE'
end

-------------------------------------------------------------------------------
function handlers.evaluate(req)
	-- 실행할 소스 코드 준비
	local sourceCode = req.arguments.expression
	if string.sub(sourceCode, 1, 1) == '!' then
		sourceCode = string.sub(sourceCode, 2)
	else
		sourceCode = 'return (' .. sourceCode .. ')'
	end

	-- 환경 준비.
	-- 뭘 요구할지 모르니까 로컬, 업밸류, 글로벌을 죄다 복사해둔다.
	-- 우선순위는 글로벌-업밸류-로컬 순서니까
	-- 그 반대로 갖다놓아서 나중 것이 앞의 것을 덮어쓰게 한다. 
	local depth = req.arguments.frameId
	local tempG = {}
	local declared = {}
	local function set(k, v)
		tempG[k] = v
		declared[k] = true
	end

	for name, value in pairs(_G) do
		set(name, value)
	end

	if depth then
		local info = debug_getinfo(depth, 'f')
		if info and info.func then
			for i = 1, 9999 do
				local name, value = debug.getupvalue(info.func, i)
				if name == nil then break end
				set(name, value)
			end
		end

		for i = 1, 9999 do
			local name, value = debug_getlocal(depth, i)
			if name == nil then break end
			set(name, value)
		end
	else
		-- VSCode가 depth를 안 보낼 수도 있다.
		-- 특정 스택 프레임을 선택하지 않은, 전역 이름만 조회하는 경우이다.
	end
	local mt = {
		__newindex = function() error('assignment not allowed', 2) end,
		__index = function(t, k) if not declared[k] then error('not declared', 2) end end
	}
	setmetatable(tempG, mt)

	-- 파싱
	-- loadstring for Lua 5.1
	-- load for Lua 5.2 and 5.3(supports the private environment's load function)
	local fn, err = (loadstring or load)(sourceCode, 'X', nil, tempG)
	if fn == nil then
		sendFailure(req, string.gsub(err, '^%[string %"X%"%]%:%d+%: ', ''))
		return
	end

	-- 실행하고 결과 송신
	if setfenv ~= nil then
		-- Only for Lua 5.1
		setfenv(fn, tempG)
	end

	local success, aux = pcall(fn)
	if not success then
		aux = aux or '' -- Execution of 'error()' returns nil as aux
		sendFailure(req, string.gsub(aux, '^%[string %"X%"%]%:%d+%: ', ''))
		return
	end

	local varNameCount = {}
	local item = registerVar(varNameCount, '', aux)

	sendSuccess(req, {
		result = item.value,
		type = item.type,
		variablesReference = item.variablesReference
	})
end

-------------------------------------------------------------------------------
return debuggee
