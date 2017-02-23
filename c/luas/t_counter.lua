local counter = require("lm_counter")
local tsocket = require("lm_tsocket")

local c = counter.new(0, "c1")
c:add(4)
print("val=" .. c:getval())
print(c)

local tsock = tsocket.get()
print(tsock)
n, s, err = tsock:read(10)
print(n, s, err)

return 99
