local tsocket = require("lm_tsocket")
local base = require("artorias.socket.base")


local _tsocket = {}


function _tsocket:new()
    local obj = {_socket=tsocket.get()}
    self.__index = self
    return setmetatable(obj, self)
end


function _tsocket:read(size)
    return self._socket:read(size)
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
