-- This file implements waitSeconds, waitSignal, signal, and their supporting stuff.

-- This table is indexed by coroutine and simply contains the time at which the coroutine
-- should be woken up.
local WAITING_ON_TIME = {}

-- This table is indexed by signal and contains list of coroutines that are waiting
-- on a given signal
local WAITING_ON_SIGNAL = {}

-- Keep track of how long the game has been running.
local CURRENT_TIME = 0

wait_support = {}

function wait_support.clear_waiting_threads()
	for k, _ in pairs(WAITING_ON_TIME) do
  		WAITING_ON_TIME[k] = nil
	end

	CURRENT_TIME = 0
end

function wait_support.wake_up_waiting_threads(currentTime)
    -- This function should be called once per game logic update with the amount of time
    -- that has passed since it was last called
    CURRENT_TIME = currentTime

    -- First, grab a list of the threads that need to be woken up. They'll need to be removed
    -- from the WAITING_ON_TIME table which we don't want to try and do while we're iterating
    -- through that table, hence the list.
    local threadsToWake = {}
    for co, wakeupTime in pairs(WAITING_ON_TIME) do
        if wakeupTime < CURRENT_TIME then
            table.insert(threadsToWake, co)
        end
    end

    -- Now wake them all up.
    for _, co in ipairs(threadsToWake) do
        WAITING_ON_TIME[co] = nil -- Setting a field to nil removes it from the table
        coroutine.resume(co)
    end
end

function wait_support.wait_seconds(seconds)
    -- Grab a reference to the current running coroutine.
    local co = coroutine.running()

    -- If co is nil, that means we're on the main process, which isn't a coroutine and can't yield
    assert(co ~= nil, "The main thread cannot wait!")

    -- Store the coroutine and its wakeup time in the WAITING_ON_TIME table
    local wakeupTime = CURRENT_TIME + seconds
    WAITING_ON_TIME[co] = wakeupTime

    -- And suspend the process
    return coroutine.yield(co)
end

function wait_support.wait_signal(signalName)
    -- Same check as in waitSeconds; the main thread cannot wait
    local co = coroutine.running()
    assert(co ~= nil, "The main thread cannot wait!")

    if WAITING_ON_SIGNAL[signalStr] == nil then
        -- If there wasn't already a list for this signal, start a new one.
        WAITING_ON_SIGNAL[signalName] = { co }
    else
        table.insert(WAITING_ON_SIGNAL[signalName], co)
    end

    return coroutine.yield()
end

function wait_support.signal(signalName)
    local threads = WAITING_ON_SIGNAL[signalName]
    if threads == nil then return end

    WAITING_ON_SIGNAL[signalName] = nil
    for _, co in ipairs(threads) do
        coroutine.resume(co)
    end
end

function wait_support.start(func)
    -- This function is just a quick wrapper to start a coroutine.
    local co = coroutine.create(func)
    return coroutine.resume(co)
end
