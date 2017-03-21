local tsocket = require("lm_tsocket")
local base = require("artorias.socket.base")


local _tsocket = {}


function _tsocket:new()
    local sock = tsocket.get()
    local obj = {_socket=sock}
    self.__index = self
    return setmetatable(obj, self)
end


function _tsocket:ready_for_read()
    return coroutine.yield(tsocket.WAIT_FOR_INPUT)
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

    if err == tsocket.EAGAIN and nbyte ~= 0 then
        err = nil
    elseif err == nil and nbyte == 0 then
        err = "conn close"
    else
        err = "errno: " .. err
    end

    return nbyte, s, err
end


function _tsocket:send(buf, size)
    return self._socket:send(buf, size)
end


local _M = {}


_M.version = base.version


function _M.get()
    return _tsocket:new()
end


return _M
