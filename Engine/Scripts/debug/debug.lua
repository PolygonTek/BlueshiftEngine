-- This is a one-line comment that ends at the end of the line
--[[ This a multi-line (long) [comment](http://www.lua.org/manual/5.1/manual.html#2.1)
     that ends with this closing bracket --> ]]
--[=[ This is also a long comment ]=]
local server = ...
local home = debug.getinfo(1).source
--local blueshift = require "blueshift"
--blueshift.log(home)

home = string.match(home, "(.-)([^\\/]*)$", 1)
home = string.sub(home, 2);

package.path = ""
package.path = string.format("%s;%s?.lua", package.path, home)
package.path = string.format("%s;%s?/?.lua", package.path, home)

local function searcher_Lua(modulename)
  local errmsg = ""
  -- Find source
  local modulepath = string.gsub(modulename, "%.", "/")
  for path in string.gmatch(package.path, "([^;]+)") do
    local filename = string.gsub(path, "%?", modulepath)
    local file = _G["blueshift.io"].open(filename, "rb")
    if file then
      -- Compile and return the module
      return assert(load(assert(file:read("*a")), '@' .. filename))
    end
    errmsg = errmsg.."\n\tno file '"..filename.."' (checked with custom loader)"
  end
  return errmsg
end

-- Install the loader 
package.searchers[2] = searcher_Lua

local mobdebug = require('mobdebug')
mobdebug.onexit = mobdebug.done
local result = mobdebug.start(server) 
--return result
