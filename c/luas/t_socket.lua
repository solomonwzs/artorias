local tsocket = require("lm_tsocket")
local math = require("math")

local tsock = tsocket.get()
local r = math.random(0, 1)

if r == 0 then
    local n, s, err = tsock:read(1024)
    if err ~= nil then
        return -1
    end
    print("===")
    print(s)
end

local r = "+OK\r\n"
n, err = tsock:send(r, #r)
if err ~= nil then
    return -1
end
