local queue = {}

function queue.new()
	return {}
end

function queue.empty(q)
	return not q._top
end

function queue.push(q, value)
	if queue.empty(q) then
		q._top = { value = value }
		q._bottom = q._top
	else
		q._bottom.below = { value = value }
		q._bottom = q._bottom.below
	end
end

function queue.top(q)
	if queue.empty(q) then
		return
	end
	return q._top.value
end

function queue.pop(q)
	if queue.empty(q) then
		return
	end
	local top_element = queue.top(q)
	q._top = q._top.below
	if queue.empty(q) then -- List became empty through popping
		q._bottom = nil
	end
	return top_element
end

return queue