local lcounter = require("lcounter")

local c = lcounter.new(0, "c1")
c:add(4)
print("val=" .. c:getval())
print(c)
