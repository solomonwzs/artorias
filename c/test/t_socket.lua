local tsocket = require("artorias.socket.tsocket")

local tsock = tsocket.get()
coroutine.yield()

while true do
    local n, s, err = tsock:read(1024)
    if err ~= nil then
        return -1
    end

    local r = "+OK\r\n"
    n, err = tsock:send(r, #r)
    if err ~= nil then
        return -1
    end
    coroutine.yield()
end
