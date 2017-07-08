local lm_socket = require("lm_socket")
local base = require("lm_base")
local socket_base = require("artorias.socket.base")


local _socket = {}


function _socket:get_msock()
    local msock = lm_socket.get_msock()
    local res = msock:get_res()
    local obj = {_socket=msock, _res=res}
    self.__index = self
    return setmetatable(obj, self)
end


function _socket:ready_for_read(timeout)
    local typ, res, io = coroutine.yield(base.YIELD_FOR_IO, self._res,
        base.WAIT_FOR_INPUT, timeout)
    return typ == base.RESUME_IO and res == self._res
    and io == base.READY_TO_INPUT
end


function _socket:read()
    local nbyte = 0
    local s = ""
    local err = nil
    local n, buf
    repeat
        n, buf, err = self._socket:read(1024)
        if err == nil then
            s = s .. buf
            nbyte = nbyte + n
        end
    until err ~= nil or n == 0

    if (err == base.EAGAIN or err == nil) and nbyte ~= 0 then
        err = nil
    elseif err == nil and nbyte == 0 then
        err = "conn close"
    else
        err = "errno: " .. err
    end

    return nbyte, s, err
end


function _socket:send(buf)
    local err = nil
    local n = 0
    local nbyte = 0
    local typ, res, io
    while err == nil and #buf > 0 do
        n, err = self._socket:send(buf)
        if err == nil then
            nbyte = nbyte + n
            buf = buf:sub(n + 1)
        elseif err == base.EAGAIN then
            typ, res, io = coroutine.yield(base.YIELD_FOR_IO, self._res,
                base.WAIT_FOR_OUTPUT, 15)
            if typ ~= base.RESUME_IO or res ~= self._res
                or io ~= base.READY_TO_OUTPUT then
                return nbyte, "status error"
            else
                err = nil
            end
        else
            return nbyte, "error: " .. err
        end
    end
    return nbyte, nil
end


local _M = {}


_M.version = socket_base.version


function _M.get_msock()
    return _socket:get_msock()
end


return _M
