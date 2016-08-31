package redis

const (
	_SM_ERROR int = iota
	_SM_INIT
)

type command struct {
	argn int
	argv []string
}

type connState struct {
	buf []byte
}

type stateMachine struct {
	state int
}

func (sm *stateMachine) parse(str string) {
}

func (cs *connState) getToken() []byte {
	for i := 0; i < len(cs.buf)-1; i++ {
		if cs.buf[i] == '\r' && cs.buf[i+1] == '\n' {
			ret := cs.buf[:i]
			cs.buf = cs.buf[i:]
			return ret
		}
	}
	return nil
}
