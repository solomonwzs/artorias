package socketframe

import (
	"io"
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

type Worker struct {
	eventChannel chan *event
}

func NewWorker(eventChannel chan *event) *Worker {
	return &Worker{
		eventChannel: eventChannel,
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

		eventChannel := make(chan *event, 16)
		go handle(conn, eventChannel, protocol)
	}
}

func readFromConn(conn net.Conn, eventChannel chan *event) {
	for {
		buf := make([]byte, 1024)
		n, err := conn.Read(buf)

		if err == io.EOF {
			logger.Log(logger.DEBUG, "exit")
			eventChannel <- newEvent(_EVENT_CONN_CLOSE, nil)
			break
		} else if err != nil {
			logger.Log(logger.ERROR, "Error reading: ", err)
			continue
		}
		logger.Log(logger.DEBUG, buf[:n])
		eventChannel <- newEvent(_EVENT_READ_BYTE_FROM_CONN, buf[:n])
	}
}

func handle(conn net.Conn, eventChannel chan *event, protocol Protocol) {
	state := protocol.Init()

	go func() {
		readFromConn(conn, eventChannel)
	}()

	for event := range eventChannel {
		var replay *ProtocolReplay

		switch event.flag {
		case _EVENT_READ_BYTE_FROM_CONN:
			replay = protocol.HandleBytes(event.content.([]byte), state)
		case _EVENT_CUSTOM_INFO:
			replay = protocol.HandleInfo(event.content, state)
		case _EVENT_CONN_CLOSE:
			protocol.Terminal(TERMINAL_CONN_CLOSE, nil, state)
			return
		default:
			continue
		}

		state = replay.NewState
		if replay.Replay {
			conn.Write(replay.Result)
		}
	}
}

func handleDefer(state interface{}, conn net.Conn, protocol Protocol) {
	if err := recover(); err != nil {
		protocol.Terminal(TERMINAL_PANIC, err, state)
	}
}
