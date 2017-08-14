local lm_socket = require("lm_socket")
local lm_base = require("lm_base")

local msock = lm_socket.get_msock()
local sock = lm_socket.new("127.0.0.1", 6379)

local n = 0
local str = ""
local err = nil

local typ = 0
local so = nil
local rw = 0

-- while true do
--     n, s, err = msock:read(1024)
--     if err ~= nil or (err == nil and n == 0) then
--         break
--     end
--     if (s == "*1\r\n$7\r\nCOMMAND\r\n") then
--         msock:send(r)
--     else
--         n, err = sock:send(s)
--         if err ~= nil then
--             break
--         end
-- 
--         n, s, err = sock:read(1024)
--         if err ~= nil then
--             break
--         end
-- 
--         n, err = msock:send(s)
--         if err ~= nil then
--             break
--         end
--     end
-- end

lm_socket.ev_begin(2,
    msock, lm_base.EV.EPOLLIN | lm_base.EV.EPOLLOUT |lm_base.EV.EPOLLET,
    sock, lm_base.EV.EPOLLIN | lm_base.EV.EPOLLOUT |lm_base.EV.EPOLLET
    )

while true do
    typ, so, rw = lm_socket.ev_wait(11)
    print(typ, so, rw)
end

lm_socket.ev_end()
