local lm_socket = require("lm_socket")
local base = require("lm_base")

local msock = lm_socket.get_msock()

local typ, res, io = coroutine.yield(base.YIELD_FOR_IO,
    msock:get_res(), lm_socket.WAIT_FOR_INPUT, 15)
while typ == base.RESUME_IO and res == msock:get_res() and
io == base.READY_TO_INPUT do
    local n, s, err = msock:read(0)
    if err ~= nil or (err == nil and n == 0) then
        break
    end

    local r = "+OK\r\n"
    n, err = msock:send(r)
    if err ~= nil then
        break
    end

    local typ, res, io = coroutine.yield(base.YIELD_FOR_IO,
        msock:get_res(), base.WAIT_FOR_INPUT, 15)
end
