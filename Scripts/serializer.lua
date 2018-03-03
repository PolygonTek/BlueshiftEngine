local blueshift = require "blueshift"

local serializer = {}

function serializer.serialize(val, name, skipnewlines, depth)
	skipnewlines = skipnewlines or false
  	depth = depth or 0

  	local tmp = string.rep(" ", depth)

  	if name then
  		if type(name) == "number" then
  			tmp = tmp .. "[" .. name .. "]" .. " = "
  		else
      		tmp = tmp .. name .. " = " 
    	end
  	end

  	if type(val) == "table" then
    	tmp = tmp .. "{" .. (not skipnewlines and "\n" or "")

    	for k, v in pairs(val) do
      		tmp =  tmp .. serializer.serialize(v, k, skipnewlines, depth + 1) .. "," .. (not skipnewlines and "\n" or "")
    	end

    	tmp = tmp .. string.rep(" ", depth) .. "}"
  	elseif type(val) == "number" then
    	tmp = tmp .. tostring(val)
  	elseif type(val) == "string" then
    	tmp = tmp .. string.format("%q", val)
  	elseif type(val) == "boolean" then
    	tmp = tmp .. (val and "true" or "false")
  	else
    	tmp = tmp .. "\"[inserializeable datatype:" .. type(val) .. "]\""
  	end

  	return tmp
end

function serializer.deserialize(s)
  -- tricky way to determine lua version
  if _ENV then 
  	return load("return"..s)()
  else
    return loadstring("return"..s)()
  end
end

return serializer