package socketframe

import (
	"io"
	"logger"
	"net"
	"os"
	"time"
)

type ProtocolReplay struct {
	Replay   bool
	Result   []byte
	NewState interface{}
}

type Protocol interface {
	Init() (state interface{})
	Handle(buf []byte, state interface{}) *ProtocolReplay
}

func StartListener(listener net.Listener, protocol Protocol) {
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
		go handle(conn, protocol)
	}
}

func handle(conn net.Conn, protocol Protocol) {
	buf := make([]byte, 1024)
	logger.Log(logger.DEBUG, "connect")
	state := protocol.Init()
	for {
		n, err := conn.Read(buf)

		if err == io.EOF {
			logger.Log(logger.DEBUG, "exit")
			break
		} else if err != nil {
			logger.Log(logger.ERROR, "Error reading: ", err.Error())
			continue
		}
		replay := protocol.Handle(buf[:n], state)
		state = replay.NewState
		if replay.Replay {
			conn.Write(replay.Result)
		}
	}
	conn.Close()
}
