package main

import (
	"fmt"
	"logger"
	"net"
	"os"
	"protocol/redis"
	"socketframe"
)

const (
	CONN_HOST = "localhost"
	CONN_PORT = "3333"
	CONN_TYPE = "tcp"
)

func main() {
	logger.Init()
	logger.AddLogger("default", nil)

	listener, err := net.Listen(CONN_TYPE,
		fmt.Sprintf("%s:%s", CONN_HOST, CONN_PORT))
	if err != nil {
		logger.Log(logger.ERROR, err)
		os.Exit(1)
	}
	defer listener.Close()

	socketframe.NewSocketServer(listener, &redis.Redis{})
}
