local lm_socket = require("lm_socket")
local lm_base = require("lm_base")

local proxy = lm_socket.get_msock()
local target = lm_socket.new("127.0.0.1", 6379)

local to_proxy = ""
local to_target = ""

local n = 0
local str = ""
local err = nil

local typ = 0
local sock = nil
local rw = 0

function read_all(so)
    local nbyte = 0
    local s = ""
    local err = nil
    local n, buf
    repeat
        n, buf, err = so:sread(1024)
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

lm_socket.ev_begin(2,
    proxy, lm_base.EV.EPOLLIN | lm_base.EV.EPOLLET,
    target, lm_base.EV.EPOLLIN | lm_base.EV.EPOLLET
    )

while true do
    typ, sock, rw = lm_socket.ev_wait(60)
    if typ == lm_base.S.RESUME_IO then
        if sock == proxy then
            n, str, err = read_all(proxy)
            if err ~= nil then
                break
            end
            to_target = to_target .. str

            n, err = target:ssend(to_target)
            to_target = ""
        else
            n, str, err = read_all(target)
            if err ~= nil then
                break
            end
            to_proxy = to_proxy .. str

            n, err = proxy:ssend(to_proxy)
            to_proxy = ""
        end
    end
end

lm_socket.ev_end()

target:close()
