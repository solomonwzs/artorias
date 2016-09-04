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
	conn         net.Conn
	eventChannel chan *event
}

func newWorker(conn net.Conn, eventChannel chan *event) *Worker {
	return &Worker{
		conn:         conn,
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

		worker := newWorker(conn, make(chan *event, 16))
		go handle(worker, protocol)
	}
}

func readFromConn(worker *Worker) {
	for {
		buf := make([]byte, 1024)
		n, err := worker.conn.Read(buf)

		if err == io.EOF {
			logger.Log(logger.DEBUG, "exit")
			worker.eventChannel <- newEvent(_EVENT_CONN_CLOSE, nil)
			break
		} else if err != nil {
			logger.Log(logger.ERROR, "Error reading: ", err)
			continue
		}
		logger.Log(logger.DEBUG, buf[:n])
		worker.eventChannel <- newEvent(_EVENT_READ_BYTE_FROM_CONN, buf[:n])
	}
}

func handle(worker *Worker, protocol Protocol) {
	state := protocol.Init(worker)

	go func() {
		readFromConn(worker)
	}()

	for event := range worker.eventChannel {
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
			worker.conn.Write(replay.Result)
		}
	}
}

func handleDefer(state interface{}, conn net.Conn, protocol Protocol) {
	if err := recover(); err != nil {
		protocol.Terminal(TERMINAL_PANIC, err, state)
	}
}
