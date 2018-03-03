local blueshift = require "blueshift"
local serializer = require "serializer"
local FileSystem = blueshift.FileSystem
local File = blueshift.File
local Str = blueshift.Str

local savedata = {}

local function deepcopy(o, seen)
    seen = seen or {}
    if o == nil then return nil end
    if seen[o] then return seen[o] end
  
    local no
    if type(o) == 'table' then
        no = {}
        seen[o] = no
  
        for k, v in next, o, nil do
            no[deepcopy(k, seen)] = deepcopy(v, seen)
        end
        setmetatable(no, deepcopy(getmetatable(o), seen))
    else -- number, string, boolean, etc
        no = o
    end
    return no
end

function savedata.load(path)
    -- load saved data
    local file = FileSystem.open(path, File.Mode.ReadMode, false)
    if not file then
        return nil
    end
    local s = Str()
    file:read_string(s)
    FileSystem.close(file)

    return serializer.deserialize(s:c_str())
end

function savedata.save(path, data)
    local s = serializer.serialize(data)

    local file = FileSystem.open(path, File.Mode.WriteMode, false)
    file:write_string(s)
    FileSystem.close(file)
end

return savedata