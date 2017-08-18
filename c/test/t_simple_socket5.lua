local lm_base = require("lm_base")
local lm_socket = require("lm_socket")

local proxy = lm_socket.get_msock()
local target = nil

local n = 0
local buf = ""
local err = nil

function read_all(sock)
    local nbyte = 0
    local s = ""
    local err = nil
    local n, buf
    repeat
        n, buf, err = sock:sread(1024)
        if err == nil then
            s = s .. buf
            nbyte = nbyte + n
        end
    until err ~= nil or n == 0

    if (err == lm_base.E.EAGAIN or err == nil) and nbyte ~= 0 then
        err = nil
    elseif err == nil and nbyte == 0 then
        err = "conn close"
    else
        err = "errno: " .. err
    end

    return nbyte, s, err
end

n, buf, err = proxy:read(1024)
if err ~= nil then
    print(err)
    return
end

if string.byte(buf, 1) == 0x05 then
    n, err = proxy:send("\x05\x00")
end
