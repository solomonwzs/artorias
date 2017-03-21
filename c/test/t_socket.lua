local tsocket = require("artorias.socket.tsocket")

local tsock = tsocket.get()

while tsock:ready_for_read() do
    local n, s, err = tsock:read()
    if err ~= nil then
        return
    end

    local r = "+OK\r\n"
    n, err = tsock:send(r, #r)
    if err ~= nil then
        return
    end
end
