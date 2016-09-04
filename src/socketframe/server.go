package socketframe

import (
	"io"
	"logger"
	"net"
	"os"
	"sync"
	"time"
)

const (
	_EVENT_READ_BYTE_FROM_CONN = iota
	_EVENT_CONN_CLOSE
	_EVENT_CUSTOM_INFO
)

const (
	TERMINAL_CONN_CLOSE = iota
	TERMINAL_NORMAL
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

type ProtocolReplay struct {
	Replay   bool
	Result   []byte
	NewState interface{}
}

type Protocol interface {
	Init() (state interface{})
	HandleBytes(buf []byte, state interface{}) *ProtocolReplay
	HandleInfo(info interface{}, state interface{}) *ProtocolReplay
	Terminal(reason int, err error, state interface{})
}

type SocketServer struct {
	eventChannel chan *event
	wg           sync.WaitGroup
}

func NewSocketServer(listener net.Listener, protocol Protocol) *SocketServer {
	logger.Log(logger.DEBUG, "Start Listen")

	server := &SocketServer{
		eventChannel: make(chan *event, 16),
	}

	var tempDelay time.Duration = 0
	go func() {
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
			server.wg.Add(1)
			go server.handle(conn, protocol)
		}
		server.wg.Wait()
	}()

	return server
}

func (server *SocketServer) readFromConn(conn net.Conn) {
	for {
		buf := make([]byte, 1024)
		n, err := conn.Read(buf)

		if err == io.EOF {
			logger.Log(logger.DEBUG, "exit")
			server.eventChannel <- newEvent(_EVENT_CONN_CLOSE, nil)
			break
		} else if err != nil {
			logger.Log(logger.ERROR, "Error reading: ", err.Error())
			continue
		}
		server.eventChannel <- newEvent(_EVENT_READ_BYTE_FROM_CONN, buf[:n])
	}
}

func (server *SocketServer) handle(conn net.Conn, protocol Protocol) {
	defer server.wg.Done()

	go func() {
		server.readFromConn(conn)
	}()

	state := protocol.Init()
	for event := range server.eventChannel {
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
