package main

import (
	"fmt"
	"logger"
	"net"
	"os"
	"protocol/redis"
	"socketframe/client"
	"socketframe/server"
)

const (
	CONN_HOST = "localhost"
	CONN_PORT = "3333"
	CONN_TYPE = "tcp"
)

type tcpConnDialer struct{}

func (d *tcpConnDialer) NewConn() (net.Conn, error) {
	conn, err := net.Dial("tcp", "localhost:6379")
	return conn, err
}

func (d *tcpConnDialer) DelConn(conn net.Conn) {
	conn.Close()
}

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
	pool := client.NewPool(&tcpConnDialer{}, 1)

	conn0, err := pool.GetConn()
	logger.Log(logger.DEBUG, conn0, err)

	conn1, err := pool.GetConn()
	logger.Log(logger.DEBUG, conn1, err)

	pool.RecoverConn(conn0)
	pool.RecoverConn(conn1)
}

func main() {
	logger.Init()
	logger.AddLogger("default", nil)

	clientTest()
	for {
	}
}
