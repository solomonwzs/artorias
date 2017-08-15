local socket = require("artorias.socket.socket")

local msock = socket.get_msock()
local sock = socket.new_sock("127.0.0.1", 6379)

local n = 0
local s = ""
local err = nil

-- local r = ""
-- for i = 1, 100000 do
--     r = r .. '0'
-- end
-- r = "+" .. r .. "\r\n"
local r = "+OK\r\n"
local cmd_get = "*2\r\n$3\r\nget\r\n$1\r\na\r\n"

n, err = sock:send(cmd_get)
n, s, err = sock:read(1024)

while true do
    n, s, err = msock:read(1024)
    if err ~= nil or (err == nil and n == 0) then
        break
    end
    if (s == "*1\r\n$7\r\nCOMMAND\r\n") then
        msock:send(r)
    else
        n, err = sock:send(s)
        if err ~= nil then
            break
        end

        n, s, err = sock:read(1024)
        if err ~= nil then
            break
        end

        n, err = msock:send(s)
        if err ~= nil then
            break
        end
    end
end

sock:close()
