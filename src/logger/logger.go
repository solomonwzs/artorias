package logger

import (
	"fmt"
	"time"
)

const (
	FINEST int = iota
	FINE
	DEBUG
	TRACE
	INFO
	WARNING
	ERROR
	CRITICAL
)

func (l *LogRecord) File() string {
	return l.file
}

func (l *LogRecord) Line() int {
	return l.line
}

func (l *LogRecord) Created() time.Time {
	return l.created
}

func (l *LogRecord) Message() string {
	return l.message
}

func (l *LogRecord) Level() int {
	return l.level
}

func Init() {
	if ctrlChannel == nil {
		ctrlChannel = make(chan *ctrlMessage, _CTRL_MSG_UPPER_LIMIT)
		logChannels = make(map[string]chan *LogRecord)
		go func() {
			for msg := range ctrlChannel {
				switch msg.Message {
				case _CTRL_WRITE_LOG:
					broadcast(msg.LR)
				case _CTRL_ADD_LOGGER:
					addLogger(msg.LogChannelID, msg.LogerFunc, 16)
				case _CTRL_DEL_LOGGER:
					delLogger(msg.LogChannelID)
				default:
				}
			}
		}()
	}
}

func Log(level int, message ...interface{}) {
	msg := &ctrlMessage{
		Message: _CTRL_WRITE_LOG,
		LR:      newLogRecord(2, fmt.Sprintln(message...), level),
	}
	ctrlChannel <- msg
}

func Logf(level int, format string, message ...interface{}) {
	msg := &ctrlMessage{
		Message: _CTRL_WRITE_LOG,
		LR:      newLogRecord(2, fmt.Sprintf(format, message...), level),
	}
	ctrlChannel <- msg
}

func AddLogger(id string, f func(*LogRecord)) {
	msg := &ctrlMessage{
		Message:      _CTRL_ADD_LOGGER,
		LogChannelID: id,
		LogerFunc:    f,
	}
	ctrlChannel <- msg
}

func DelLogger(id string) {
	msg := &ctrlMessage{
		Message:      _CTRL_DEL_LOGGER,
		LogChannelID: id,
	}
	ctrlChannel <- msg
}
