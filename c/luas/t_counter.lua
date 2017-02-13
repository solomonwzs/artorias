local counter = require("lm_counter")

local c = counter.new(0, "c1")
c:add(4)
print("val=" .. c:getval())
print(c)
