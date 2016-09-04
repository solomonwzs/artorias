package socketframe

const (
	TERMINAL_CONN_CLOSE = iota
	TERMINAL_NORMAL
	TERMINAL_PANIC
)

const (
	R_REPLY = iota
	R_NOREPLY
	R_TERMINAL
)

type ProtocolReply struct {
	reply       int
	resultBytes []byte
	newState    interface{}
}

type Protocol interface {
	Init(worker *Worker) (state interface{})
	HandleBytes(buf []byte, state interface{}) *ProtocolReply
	HandleInfo(info interface{}, state interface{}) *ProtocolReply
	Terminal(reason int, err interface{}, state interface{})
}
