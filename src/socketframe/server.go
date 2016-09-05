package socketframe

import (
	"logger"
	"net"
	"os"
	"time"
)

const (
	_EVENT_READ_BYTE_FROM_CONN = iota
	_EVENT_CONN_CLOSE
	_EVENT_CUSTOM_INFO
)

type event struct {
	flag    int
	content interface{}
}

func newEvent(flag int, content interface{}) *event {
	return &event{
		flag:    flag,
		content: content,
	}
}

func NewSocketServer(listener net.Listener, protocol Protocol) {
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
				logger.Logf(logger.ERROR,
					"Accept error: %v; retrying in %v\n", err, tempDelay)
				time.Sleep(tempDelay)
				continue
			}
			os.Exit(1)
		}
		tempDelay = 0

		worker := newWorker(conn, make(chan *event, 16))
		go handle(worker, protocol)
	}
}
