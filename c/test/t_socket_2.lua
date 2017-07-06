local socket = require("artorias.socket.socket")

local msock = socket.get_msock()
local n = 0
local s = ""
local err = nil
local r = "+OK\r\n"

while msock:ready_for_read(15) do
    n, s, err = msock:read(0)
    if err ~= nil or (err == nil and n == 0) then
        break
    end

    n, err = msock:send(r)
    if err ~= nil then
        break
    end
end
