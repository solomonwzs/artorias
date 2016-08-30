package main

import (
	"logger"
	"socketframe"
)

func main() {
	logger.Init()
	logger.AddLogger("default", nil)

	socketframe.StartListener()
}
