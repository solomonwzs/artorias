package server

import (
	"errors"
	"io"
	"logger"
	"net"
)

var (
	WORKER_NOT_ALIVE = errors.New("worker not alive")
)

type Worker struct {
	conn         net.Conn
	eventChannel chan *event
	alive        bool
}

func newWorker(conn net.Conn, eventChannel chan *event) *Worker {
	return &Worker{
		conn:         conn,
		eventChannel: eventChannel,
		alive:        true,
	}
}

func SendInfoTo(worker *Worker, info interface{}) error {
	if worker.alive {
		worker.eventChannel <- newEvent(_EVENT_CUSTOM_INFO, info)
		return nil
	} else {
		return WORKER_NOT_ALIVE
	}
}

func handle(worker *Worker, protocol Protocol) {
	logger.Log(logger.DEBUG, "connect ok")

	var terminalRea int = TERMINAL_UNKNOWN
	var terminalErr interface{} = nil
	var state interface{} = nil

	defer func() {
		if err := recover(); err != nil {
			protocol.Terminal(TERMINAL_PANIC, err, state)
		} else {
			protocol.Terminal(terminalRea, terminalErr, state)
		}
		worker.alive = false
		worker.conn.Close()
		logger.Log(logger.DEBUG, "connect close")
	}()

	initState := protocol.Init(worker)
	if initState.flag == _I_TERMINAL {
		terminalRea, terminalErr = TERMINAL_INIT, nil
		return
	}
	state = initState.state

	go func() {
		readBytes(worker)
	}()

	for event := range worker.eventChannel {
		var reply *ProtocolReply

		switch event.flag {
		case _EVENT_READ_BYTE_FROM_CONN:
			reply = protocol.HandleBytes(event.content.([]byte), state)
		case _EVENT_CUSTOM_INFO:
			reply = protocol.HandleInfo(event.content, state)
		case _EVENT_CONN_CLOSE:
			terminalRea, terminalErr = TERMINAL_CONN_CLOSE, nil
			return
		default:
			continue
		}

		state = reply.newState
		switch reply.reply {
		case _R_TERMINAL:
			terminalRea, terminalErr = TERMINAL_NORMAL, nil
			return
		case _R_REPLY:
			worker.conn.Write(reply.replyBytes)
		case _R_NOREPLY:
		default:
		}
	}
}

func readBytes(worker *Worker) {
	for worker.alive {
		buf := make([]byte, 1024)
		n, err := worker.conn.Read(buf)

		if err == io.EOF {
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
