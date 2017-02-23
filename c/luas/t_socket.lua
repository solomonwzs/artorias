local tsocket = require("lm_tsocket")

local tsock = tsocket.get()

local n, s, err = tsock:read(1024)
if err ~= nil then
    return -1
end

local r = "+OK\r\n"
n, err = tsock:send(r, #r)
if err ~= nil then
    return -1
end