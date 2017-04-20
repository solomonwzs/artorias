local tsocket = require("artorias.socket.tsocket")
local base = require("lm_base")
local lm_socket = require("lm_socket")

local tsock = tsocket.get()
local sock = lm_socket.new("127.0.0.1", 6379, 0)

while tsock:ready_for_read() do
    local n, s, err = tsock:read()
    if err ~= nil then
        break
    end

    n, err = sock:send("*2\r\n$3\r\nGET\r\n$1\r\na\r\n")
    print("->")
    sock:set_event(lm_socket.SOCK_EVENT_IN)
    coroutine.yield(base.WAIT_FOR_INPUT)
    n, s, err = sock:read(1024)
    print("<-")
    sock:set_event(lm_socket.SOCK_EVENT_NONE)

    local r = "+OK\r\n"
    n, err = tsock:send(r)
    if err ~= nil then
        break
    end
end

sock:close()
