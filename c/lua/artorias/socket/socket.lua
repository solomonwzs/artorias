local lm_socket = require("lm_socket")
local base = require("lm_base")
local socket_base = require("artorias.socket.base")

local _socket = {}

function _socket:get_msock()
  local msock = lm_socket.get_msock()
  local res = msock:get_res()
  local obj = {_socket = msock, _res = res}
  self.__index = self
  return setmetatable(obj, self)
end

function _socket:new_sock(host, port)
  local sock = lm_socket.new(host, port)
  local res = sock:get_res()
  local obj = {_socket = sock, _res = res}
  self.__index = self
  return setmetatable(obj, self)
end

function _socket:ready_for_read(timeout)
  local typ, res, io = coroutine.yield(base.S.YIELD_FOR_IO, self._res,
                                       base.S.WAIT_FOR_INPUT, timeout)
  return typ == base.S.RESUME_IO and res == self._res and io ==
             base.S.READY_TO_INPUT
end

function _socket:read(n) return self._socket:read(n, base.D.TIMEOUT_SECS); end

function _socket:read_all()
  local nbyte = 0
  local s = ""
  local err = nil
  local n, buf
  repeat
    n, buf, err = self._socket:sread(1024)
    if err == nil then
      s = s .. buf
      nbyte = nbyte + n
    end
  until err ~= nil or n == 0

  if (err == base.E.EAGAIN or err == nil) and nbyte ~= 0 then
    err = nil
  elseif err == nil and nbyte == 0 then
    err = "conn close"
  else
    err = "errno: " .. err
  end

  return nbyte, s, err
end

function _socket:send(buf) return self._socket:send(buf) end

local _M = {}

_M.version = socket_base.version

function _M.get_msock() return _socket:get_msock() end

function _M.new_sock(host, port) return _socket:new_sock(host, port) end

_M.ev_begin = lm_socket.ev_begin
_M.ev_wait = lm_socket.ev_wait
_M.ev_end = lm_socket.ev_end

_M.ev_r = base.EV.EPOLLIN | base.EV.EPOLLET
_M.ev_w = base.EV.EPOLLOUT | base.EV.EPOLLET
_M.ev_rw = base.EV.EPOLLOUT | base.EV.EPOLLOUT | base.EV.EPOLLET

return _M
