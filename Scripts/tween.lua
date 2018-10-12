local tweeners = {}

local EaseIn = 0
local EaseOut = 1
local EaseInOut = 2
local EaseOutIn = 3

local function tweenLinear(a, b, t)
	return a + (b - a) * t
end

local function tweenQuadratic(a, b, t)
	return a + (b - a) * t * t
end

local function tweenCubic(a, b, t)
	return a + (b - a) * t * t * t
end

local function tweenQuartic(a, b, t)
	return a + (b - a) * t * t * t * t
end

local function tweenQuintic(a, b, t)
	return a + (b - a) * t * t * t * t * t
end

local function tweenSinusoidal(a, b, t)
	return a + (b - a) * (1 - math.cos(math.pi * 0.5 * t))
end

local function tweenExponential(a, b, t)
	return a + (b - a) * math.pow(2, 10 * (t - 1))
end

local function tweenCircular(a, b, t)
	return a + (b - a) * (1 - math.sqrt(1 - t * t))
end

local function tweenElastic(a, b, t)
	local w = 8 * math.pi
	local z = 0.32
	local damping = math.exp(-z * w * (1 - t))
	local periodic = w * math.sqrt(1 - z * z) * (1 - t)
	return a + (b - a) * damping * math.cos(periodic)
end

local function tweenBack(a, b, t)
	return a + (b - a) * t * t * (e * t + t - math.exp(1))
end

local function tweenBounce(a, b, t)
	if t < 1 / 2.75 then
		return a + (b - a) * (7.5625 * t * t)
	elseif t < 2 / 2.75 then
		t = t - 1.5 / 2.75
		return a + (b - a) * (7.5625 * t * t + 0.75)
	elseif t < 2.5 / 2.75 then
		t = t - 2.25 / 2.75
		return a + (b - a) * (7.5625 * t * t + 0.9375)
	else
		t = t - 2.625 / 2.75
		return a + (b - a) * (7.5625 * t * t + 0.984375)
	end
end

local function tweenFunc(easeType, a, b, t)
	t = math.min(math.max(t, 0), 1)

	if easeType.easing == EaseOut or (easeType.easing == EaseInOut and t > 0.5) or (easeType.easing == EaseOutIn and t < 0.5) then
		local temp = a
		a = b
		b = temp
		t = 1 - t
	end

	if easeType.easing == EaseInOut or easeType.easing == EaseOutIn then
		if t < 0.5 then
			b = (a + b) * 0.5
			t = t * 2
		else
			a = (a + b) * 0.5
			t = (t - 0.5) * 2
		end
	end

	return easeType.func(a, b, t)
end

tween = {}

-- Linear
tween.EaseInLinear = {}
tween.EaseInLinear.easing = EaseIn
tween.EaseInLinear.func = tweenLinear

tween.EaseOutLinear = {}
tween.EaseOutLinear.easing = EaseOut
tween.EaseOutLinear.func = tweenLinear

tween.EaseInOutLinear = {}
tween.EaseInOutLinear.easing = EaseInOut
tween.EaseInOutLinear.func = tweenLinear

tween.EaseOutInLinear = {}
tween.EaseOutInLinear.easing = EaseOutIn
tween.EaseOutInLinear.func = tweenLinear

-- Quadratic
tween.EaseInQuadratic = {}
tween.EaseInQuadratic.easing = EaseIn
tween.EaseInQuadratic.func = tweenQuadratic

tween.EaseOutQuadratic = {}
tween.EaseOutQuadratic.easing = EaseOut
tween.EaseOutQuadratic.func = tweenQuadratic

tween.EaseInOutQuadratic = {}
tween.EaseInOutQuadratic.easing = EaseInOut
tween.EaseInOutQuadratic.func = tweenQuadratic

tween.EaseOutInQuadratic = {}
tween.EaseOutInQuadratic.easing = EaseOutIn
tween.EaseOutInQuadratic.func = tweenQuadratic

-- Cubic
tween.EaseInCubic = {}
tween.EaseInCubic.easing = EaseIn
tween.EaseInCubic.func = tweenCubic

tween.EaseOutCubic = {}
tween.EaseOutCubic.easing = EaseOut
tween.EaseOutCubic.func = tweenCubic

tween.EaseInOutCubic = {}
tween.EaseInOutCubic.easing = EaseInOut
tween.EaseInOutCubic.func = tweenCubic

tween.EaseOutInCubic = {}
tween.EaseOutInCubic.easing = EaseOutIn
tween.EaseOutInCubic.func = tweenCubic

-- Quartic
tween.EaseInQuartic = {}
tween.EaseInQuartic.easing = EaseIn
tween.EaseInQuartic.func = tweenQuartic

tween.EaseOutQuartic = {}
tween.EaseOutQuartic.easing = EaseOut
tween.EaseOutQuartic.func = tweenQuartic

tween.EaseInOutQuartic = {}
tween.EaseInOutQuartic.easing = EaseInOut
tween.EaseInOutQuartic.func = tweenQuartic

tween.EaseOutInQuartic = {}
tween.EaseOutInQuartic.easing = EaseOutIn
tween.EaseOutInQuartic.func = tweenQuartic

-- Quartic
tween.EaseInQuartic = {}
tween.EaseInQuartic.easing = EaseIn
tween.EaseInQuartic.func = tweenQuartic

tween.EaseOutQuartic = {}
tween.EaseOutQuartic.easing = EaseOut
tween.EaseOutQuartic.func = tweenQuartic

tween.EaseInOutQuartic = {}
tween.EaseInOutQuartic.easing = EaseInOut
tween.EaseInOutQuartic.func = tweenQuartic

tween.EaseOutInQuartic = {}
tween.EaseOutInQuartic.easing = EaseOutIn
tween.EaseOutInQuartic.func = tweenQuartic

-- Sinusoidal
tween.EaseInSinusoidal = {}
tween.EaseInSinusoidal.easing = EaseIn
tween.EaseInSinusoidal.func = tweenSinusoidal

tween.EaseOutSinusoidal = {}
tween.EaseOutSinusoidal.easing = EaseOut
tween.EaseOutSinusoidal.func = tweenSinusoidal

tween.EaseInOutSinusoidal = {}
tween.EaseInOutSinusoidal.easing = EaseInOut
tween.EaseInOutSinusoidal.func = tweenSinusoidal

tween.EaseOutInSinusoidal = {}
tween.EaseOutInSinusoidal.easing = EaseOutIn
tween.EaseOutInSinusoidal.func = tweenSinusoidal

-- Exponential
tween.EaseInExponential = {}
tween.EaseInExponential.easing = EaseIn
tween.EaseInExponential.func = tweenExponential

tween.EaseOutExponential = {}
tween.EaseOutExponential.easing = EaseOut
tween.EaseOutExponential.func = tweenExponential

tween.EaseInOutExponential = {}
tween.EaseInOutExponential.easing = EaseInOut
tween.EaseInOutExponential.func = tweenExponential

tween.EaseOutInExponential = {}
tween.EaseOutInExponential.easing = EaseOutIn
tween.EaseOutInExponential.func = tweenExponential

-- Circular
tween.EaseInCircular = {}
tween.EaseInCircular.easing = EaseIn
tween.EaseInCircular.func = tweenCircular

tween.EaseOutCircular = {}
tween.EaseOutCircular.easing = EaseOut
tween.EaseOutCircular.func = tweenCircular

tween.EaseInOutCircular = {}
tween.EaseInOutCircular.easing = EaseInOut
tween.EaseInOutCircular.func = tweenCircular

tween.EaseOutInCircular = {}
tween.EaseOutInCircular.easing = EaseOutIn
tween.EaseOutInCircular.func = tweenCircular

-- Elastic
tween.EaseInElastic = {}
tween.EaseInElastic.easing = EaseIn
tween.EaseInElastic.func = tweenElastic

tween.EaseOutElastic = {}
tween.EaseOutElastic.easing = EaseOut
tween.EaseOutElastic.func = tweenElastic

tween.EaseInOutElastic = {}
tween.EaseInOutElastic.easing = EaseInOut
tween.EaseInOutElastic.func = tweenElastic

tween.EaseOutInElastic = {}
tween.EaseOutInElastic.easing = EaseOutIn
tween.EaseOutInElastic.func = tweenElastic

-- Back
tween.EaseInBack = {}
tween.EaseInBack.easing = EaseIn
tween.EaseInBack.func = tweenBack

tween.EaseOutBack = {}
tween.EaseOutBack.easing = EaseOut
tween.EaseOutBack.func = tweenBack

tween.EaseInOutBack = {}
tween.EaseInOutBack.easing = EaseInOut
tween.EaseInOutBack.func = tweenBack

tween.EaseOutInBack = {}
tween.EaseOutInBack.easing = EaseOutIn
tween.EaseOutInBack.func = tweenBack

-- Bounce
tween.EaseInBounce = {}
tween.EaseInBounce.easing = EaseIn
tween.EaseInBounce.func = tweenBounce

tween.EaseOutBounce = {}
tween.EaseOutBounce.easing = EaseOut
tween.EaseOutBounce.func = tweenBounce

tween.EaseInOutBounce = {}
tween.EaseInOutBounce.easing = EaseInOut
tween.EaseInOutBounce.func = tweenBounce

tween.EaseOutInBounce = {}
tween.EaseOutInBounce.easing = EaseOutIn
tween.EaseOutInBounce.func = tweenBounce

function tween.clear_tweeners()
	for key, _ in pairs(tweeners) do
  		tweeners[key] = nil
	end
end

function tween.update_tweeners(deltaTime, timeScale)
	local tweenersToRemove = {}

    for key, tweener in pairs(tweeners) do
    	tweener.time = math.min(tweener.time + deltaTime * (tweener.scaledTime and timeScale or 1), tweener.duration)

    	tweener.func(tweenFunc(tweener.easeType, tweener.a, tweener.b, tweener.time / tweener.duration))

        if tweener.time == tweener.duration then
        	table.insert(tweenersToRemove, key)
        end
    end

	for _, key in ipairs(tweenersToRemove) do
		tweeners[key] = nil
	end
end

function tween.add(easeType, duration, scaledTime, a, b, func)
	func(tweenFunc(easeType, a, b, 0))

	tweeners[func] = {
		easeType = easeType,
		duration = duration,
		scaledTime = scaledTime,
		time = 0,
		a = a, 
		b = b, 
		func = func
	}

	return tweeners[func]
end

function tween.cancel(tweener)
	if tweener then
		tweeners[tweener.func] = nil
	end
end

