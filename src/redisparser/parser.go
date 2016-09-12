package redisparser

const (
	_SM_START = iota
	_SM_UNITL_CRLF
	_SM_UNITL_CRLF_END
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

type RedisCommand struct {
	_type  int
	value  *redisCommandValue
	status int
	p      *RedisCommand
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
	rc.p = rc

	return rc
}

func parserStart(r *RedisCommand, b byte) {
	switch b {
	case '+':
		r._type = RC_TYPE_STATUS
		r.status = _SM_UNITL_CRLF
		r.value._Bytes = []byte{}
	case '-':
		r._type = RC_TYPE_ERROR
		r.status = _SM_UNITL_CRLF
		r.value._Bytes = []byte{}
	case '$':
		r._type = RC_TYPE_BULK
	case ':':
		r._type = RC_TYPE_INT
	case '*':
		r._type = RC_TYPE_MULIT
	default:
		r._type = RC_TYPE_UNKNOWN
	}
}

func parserUnitlCrlf(r *RedisCommand, b byte) {
	if b == '\r' {
		r.status = _SM_UNITL_CRLF_END
	} else {
		r.value._Bytes = append(r.value._Bytes, b)
	}
}

func parserUnitlCrlfEnd(r *RedisCommand, b byte) {
	if b == '\n' {
		r.status = _SM_END
	} else {
		r.status = _SM_UNITL_CRLF
		r.value._Bytes = append(r.value._Bytes, '\r', b)
	}
}
