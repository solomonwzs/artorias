package socketframe

import (
	"fmt"
	"logger"
	"net"
	"os"
	"time"
)

const (
	CONN_HOST = "localhost"
	CONN_PORT = "3333"
	CONN_TYPE = "tcp"
)

func StartListener() {
	listener, err := net.Listen(CONN_TYPE,
		fmt.Sprintf("%s:%s", CONN_HOST, CONN_PORT))
	if err != nil {
		logger.Log(logger.ERROR, err)
		os.Exit(1)
	}
	defer listener.Close()
	logger.Log(logger.DEBUG, "Start Listen")

	var tempDelay time.Duration = 0
	for {
		conn, err := listener.Accept()
		if err != nil {
			if ne, ok := err.(net.Error); ok && ne.Temporary() {
				if tempDelay == 0 {
					tempDelay = 2 * time.Millisecond
				} else {
					tempDelay *= 2
				}

				if max := 1 * time.Second; tempDelay > max {
					tempDelay = max
				}
				logger.Logf(logger.ERROR, "Accept error: %v; retrying in %v\n", err, tempDelay)
				time.Sleep(tempDelay)
				continue
			}
			os.Exit(1)
		}
		tempDelay = 0
		go handle(conn)
	}
}

func handle(conn net.Conn) {
	buf := make([]byte, 1024)
	n, err := conn.Read(buf)
	logger.Log(logger.DEBUG, n, string(buf[0:n]))
	n, err = conn.Read(buf)
	logger.Log(logger.DEBUG, n)
	if err != nil {
		logger.Log(logger.ERROR, "Error reading: ", err.Error())
	}
	conn.Write([]byte("Hello"))
	conn.Close()
}
