package main

import (
	dialRedis "dial/redis"
	"fmt"
	"logger"
	"net"
	"os"
	"protocol/redis"
	"socketframe/server"
)

const (
	CONN_HOST = "localhost"
	CONN_PORT = "3333"
	CONN_TYPE = "tcp"
)

func serverTest() {
	listener, err := net.Listen(CONN_TYPE,
		fmt.Sprintf("%s:%s", CONN_HOST, CONN_PORT))
	if err != nil {
		logger.Log(logger.ERROR, err)
		os.Exit(1)
	}
	defer listener.Close()

	server.NewSocketServer(listener, &redis.Redis{})
}

func clientTest() {
	rcp := dialRedis.NewRedisConnPool("127.0.0.1:6379:1", 16)
	rc, err := rcp.GetConn()
	if err != nil {
		logger.Log(logger.ERROR, err)
		return
	}
	rc.Query(2, [][]byte{
		[]byte("GET"),
		[]byte("aaaaaaaaaaaaaaa"),
	})
	// rc.Query(2, [][]byte{
	// 	[]byte("SELECT"),
	// 	[]byte("1"),
	// })
}

func consoleOutput(l *logger.LogRecord) {
	if l.Level() < logger.ERROR {
		return
	}
	fmt.Println(l.Message())
}

func main() {
	logger.Init()
	// logger.AddLogger("default", nil)

	// p := redisparser.NewParser()
	// logger.Log(logger.DEBUG, p.Write([]byte("*3\r\n$3\r\nSET\r\n$5\r\nmykey\r\n$7\r")))
	// logger.Log(logger.DEBUG, p.GetCommand())
	// logger.Log(logger.DEBUG, p.Write([]byte("\nmyvalue\r\n")))
	// logger.Log(logger.DEBUG, p.GetCommand())

	// clientTest()
	serverTest()
}
