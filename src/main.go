package main

import (
	"logger"
	"time"
)

func main() {
	logger.Init()
	logger.AddLogger("default", nil)

	for {
		time.Sleep(1 * time.Second)
		logger.Log(logger.FINEST, "hello")
		logger.Log(logger.FINE, "hello")
		logger.Log(logger.DEBUG, "hello")
		logger.Log(logger.TRACE, "hello")
		logger.Log(logger.INFO, "hello")
		logger.Log(logger.WARNING, "hello")
		logger.Log(logger.ERROR, "hello")
		logger.Log(logger.CRITICAL, "hello")
	}
}
