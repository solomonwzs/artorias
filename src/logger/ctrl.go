package logger

import (
	"fmt"
	"runtime"
	"time"
)

const (
	_CTRL_WRITE_LOG int = iota
	_CTRL_ADD_LOGGER
	_CTRL_DEL_LOGGER
)

const (
	_COLOR_RED        = "\033[31m"
	_COLOR_GREEN      = "\033[32m"
	_COLOR_YELLOW     = "\033[33m"
	_COLOR_BLUE       = "\033[34m"
	_COLOR_PURPLE     = "\033[35m"
	_COLOR_LIGHT_BLUE = "\033[36m"
	_COLOR_GRAY       = "\033[37m"
	_COLOR_BLACK      = "\033[30m"
)

const (
	_CTRL_MSG_UPPER_LIMIT = 4096
)

type LogRecord struct {
	file    string
	line    int
	message string
	created time.Time
	level   int
}

type ctrlMessage struct {
	Message      int
	LogChannelID string
	LogerFunc    func(*LogRecord)
	LR           *LogRecord
}

var (
	logChannels map[string]chan *LogRecord
	debug       bool              = true
	ctrlChannel chan *ctrlMessage = nil
)

func (l *LogRecord) setFile(file string) {
	l.file = file
}

func (l *LogRecord) setLine(line int) {
	l.line = line
}

func (l *LogRecord) setMessage(message string) {
	l.message = message
}

func (l *LogRecord) setCreated(created time.Time) {
	l.created = created
}

func (l *LogRecord) setLevel(level int) {
	l.level = level
}

func newLogRecord(calldepth int, message string, level int) *LogRecord {
	l := new(LogRecord)
	l.setCreated(time.Now())
	l.setMessage(message)
	l.setLevel(level)
	if debug {
		_, file, line, ok := runtime.Caller(calldepth)
		if !ok {
			l.setFile("???")
			l.setLine(0)
		} else {
			l.setFile(file)
			l.setLine(line)
		}
	}
	return l
}

func broadcast(l *LogRecord) {
	for _, ch := range logChannels {
		ch <- l
	}
}

func addLogger(id string, f func(*LogRecord), buffer int) {
	_, exist := logChannels[id]
	if exist {
		return
	}

	ch := make(chan *LogRecord, buffer)
	logChannels[id] = ch
	if f == nil {
		f = consoleOutput
	}
	go func() {
		for l := range ch {
			f(l)
		}
	}()
}

func delLogger(id string) {
	ch, exist := logChannels[id]
	if !exist {
		return
	}
	delete(logChannels, id)
	close(ch)
}

func consoleOutput(l *LogRecord) {
	var level, color string
	switch l.Level() {
	case FINEST:
		color = _COLOR_BLACK
		level = "N"
	case FINE:
		color = _COLOR_BLUE
		level = "F"
	case DEBUG:
		color = _COLOR_GREEN
		level = "D"
	case TRACE:
		color = _COLOR_LIGHT_BLUE
		level = "T"
	case INFO:
		color = _COLOR_GRAY
		level = "I"
	case WARNING:
		color = _COLOR_YELLOW
		level = "W"
	case ERROR:
		color = _COLOR_RED
		level = "E"
	case CRITICAL:
		color = _COLOR_PURPLE
		level = "C"
	}

	file := l.File()
	short := file
	for i := len(file) - 1; i > 0; i-- {
		if file[i] == '/' {
			short = file[i+1:]
			break
		}
	}

	fmt.Printf("%s%s [%s %s:%d] \033[0m%s",
		color, l.Created().Format("15:04:05"),
		level, short, l.Line(), l.Message())
}
