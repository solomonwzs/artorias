local tsocket = require("lm_tsocket")
local base = require("artorias.socket.base")


local _tsocket = {}
local sss = "hello"


function _tsocket:new()
    local sock = tsocket.get()
    local obj = {_socket=sock}
    self.__index = self
    return setmetatable(obj, self)
end


function _tsocket:ready_for_read()
    local status = coroutine.yield(tsocket.WAIT_FOR_INPUT)
    return status == tsocket.READY_TO_INPUT
end


function _tsocket:read()
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

    if (err == tsocket.EAGAIN or err == nil) and nbyte ~= 0 then
        err = nil
    elseif err == nil and nbyte == 0 then
        err = "conn close"
    else
        err = "errno: " .. err
    end

    return nbyte, s, err
end


function _tsocket:send(buf)
    local err = nil
    local n = 0
    local nbyte = 0
    while err == nil and #buf > 0 do
        n, err = self._socket:send(buf, #buf)
        if err == nil then
            nbyte = nbyte + n
            buf = buf:sub(n + 1)
        elseif err == tsocket.EAGAIN then
            local status = coroutine.yield(tsocket.WAIT_FOR_OUTPUT)
            if status ~= tsocket.READY_TO_OUTPUT then
                return nbyte, "status error"
            end
        else
            return nbyte, "errno: " .. err
        end
    end
    return nbyte, nil
end


local _M = {}


_M.version = base.version


function _M.get()
    return _tsocket:new()
end


return _M