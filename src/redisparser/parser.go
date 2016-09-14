package redisparser

const (
	_SM_START = iota
	_SM_UNITL_CRLF
	_SM_UNITL_CRLF_END
	_SM_UNITL_CRLF_OK
	_SM_END
	_SM_ERROR
)

const (
	RC_TYPE_STATUS = iota
	RC_TYPE_ERROR
	RC_TYPE_BULK
	RC_TYPE_INT
	RC_TYPE_MULIT
	RC_TYPE_UNKNOWN
)

type redisCommandValue struct {
	_Int      int
	_Bytes    []byte
	_Commands []*RedisCommand
}

func (rcv *redisCommandValue) setInt(i int) {
	rcv._Int = i
}

func (rcv *redisCommandValue) setBytes(b []byte) {
	rcv._Bytes = b
}

func (rcv *redisCommandValue) setCommands(c []*RedisCommand) {
	rcv._Commands = c
}

type RedisCommand struct {
	_type  int
	value  *redisCommandValue
	status int
}

func NewRedisCommand() *RedisCommand {
	rc := &RedisCommand{
		_type: RC_TYPE_UNKNOWN,
		value: &redisCommandValue{
			_Int:      0,
			_Bytes:    nil,
			_Commands: nil,
		},
		status: _SM_START,
	}

	return rc
}

func (rc *RedisCommand) Parser() {
	stack := []*RedisCommand{rc}
	buf := []byte{}

	parser := func(bs []byte) {
		for _, b := range bs {
			r := stack[len(stack)-1]
			switch r.status {
			case _SM_START:
				parseStart(b, r)
			case _SM_UNITL_CRLF:
			}
		}
	}
}

func parseStart(b byte, rc *RedisCommand) {
	switch b {
	case '+':
		rc._type = RC_TYPE_STATUS
		rc.status = _SM_UNITL_CRLF
	case '-':
		rc._type = RC_TYPE_ERROR
		rc.status = _SM_UNITL_CRLF
	case '$':
		rc._type = RC_TYPE_BULK
	case ':':
		rc._type = RC_TYPE_INT
	case '*':
		rc._type = RC_TYPE_MULIT
	default:
		rc._type = RC_TYPE_UNKNOWN
	}
}

func parseUnitlCrlf(b byte, rc *RedisCommand, buf *[]byte) {
	if b == '\n' && len(*buf) > 0 && (*buf)[len(*buf)-1] == '\r' {
	}
}
