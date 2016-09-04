package socketframe

const (
	TERMINAL_CONN_CLOSE = iota
	TERMINAL_NORMAL
	TERMINAL_PANIC
)

type ProtocolReplay struct {
	Replay   bool
	Result   []byte
	NewState interface{}
}

type Protocol interface {
	Init() (state interface{})
	HandleBytes(buf []byte, state interface{}) *ProtocolReplay
	HandleInfo(info interface{}, state interface{}) *ProtocolReplay
	Terminal(reason int, err interface{}, state interface{})
}
