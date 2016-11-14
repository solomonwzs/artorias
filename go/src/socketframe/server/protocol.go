package server

const (
	TERMINAL_UNKNOWN = iota
	TERMINAL_CONN_CLOSE
	TERMINAL_NORMAL
	TERMINAL_INIT
	TERMINAL_PANIC
)

const (
	_R_REPLY = iota
	_R_NOREPLY
	_R_TERMINAL
)

const (
	_I_OK = iota
	_I_TERMINAL
)

type ProtocolInitState struct {
	flag  int
	state interface{}
}

func InitStateOK(state interface{}) *ProtocolInitState {
	return &ProtocolInitState{
		flag:  _I_OK,
		state: state,
	}
}

func InitStateTerminal() *ProtocolInitState {
	return &ProtocolInitState{
		flag: _I_TERMINAL,
	}
}

type ProtocolReply struct {
	reply      int
	replyBytes []byte
	newState   interface{}
}

func ReplyClient(replyBytes []byte, newState interface{}) *ProtocolReply {
	return &ProtocolReply{
		reply:      _R_REPLY,
		replyBytes: replyBytes,
		newState:   newState,
	}
}

func ReplyNoReply(newState interface{}) *ProtocolReply {
	return &ProtocolReply{
		reply:    _R_NOREPLY,
		newState: newState,
	}
}

func ReplyTerminal(newState interface{}) *ProtocolReply {
	return &ProtocolReply{
		reply:    _R_TERMINAL,
		newState: newState,
	}
}

type Protocol interface {
	Init(worker *Worker) *ProtocolInitState
	HandleBytes(buf []byte, state interface{}) *ProtocolReply
	HandleInfo(info interface{}, state interface{}) *ProtocolReply
	Terminal(reason int, err interface{}, state interface{})
}
