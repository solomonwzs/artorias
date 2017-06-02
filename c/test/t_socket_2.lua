local tsocket = require("artorias.socket.tsocket")
local lm_socket = require("lm_socket")

local tsock = tsocket.get()
local sock = lm_socket.new("127.0.0.1", 6379, 0)

while tsock:ready_for_read() do
    local n, s, err = tsock:read()
    if err ~= nil then
        break
    end

    local r = "+OK\r\n"
    n, err = tsock:send(r)
    if err ~= nil then
        break
    end
end
