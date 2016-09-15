package redisparser

import "strconv"

const (
	_SM_START = iota
	_SM_UNITL_CRLF
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

func (rcv *redisCommandValue) setBytes(bs []byte) {
	rcv._Bytes = bs
}

func (rcv *redisCommandValue) setCommands(c []*RedisCommand) {
	rcv._Commands = c
}

func (rcv *redisCommandValue) appendToBuffer(b byte) {
	rcv._Bytes = append(rcv._Bytes, b)
}

func (rcv *redisCommandValue) bufferEndByCtrf() bool {
	l := len(rcv._Bytes)
	return l >= 2 && rcv._Bytes[l-2] == '\r' && rcv._Bytes[l-1] == '\n'
}

func (rcv *redisCommandValue) bufferToBytes() error {
	l := len(rcv._Bytes)
	rcv._Bytes = rcv._Bytes[:l-2]
	return nil
}

func (rcv *redisCommandValue) bufferToInt() error {
	l := len(rcv._Bytes)
	i, err := strconv.Atoi(string(rcv._Bytes[:l-2]))
	if err == nil {
		rcv._Int = i
	}
	return err
}

func newRedisCommandValue() *redisCommandValue {
	return &redisCommandValue{
		_Int:      0,
		_Bytes:    []byte{},
		_Commands: nil,
	}
}

type RedisCommand struct {
	_type  int
	value  *redisCommandValue
	status int
}

func NewRedisCommand() *RedisCommand {
	rc := &RedisCommand{
		_type:  RC_TYPE_UNKNOWN,
		value:  newRedisCommandValue(),
		status: _SM_START,
	}

	return rc
}

func (rc *RedisCommand) Parser() {
	stack := []*RedisCommand{rc}

	parser := func(bs []byte) {
		for _, b := range bs {
			r := stack[len(stack)-1]
			switch r.status {
			case _SM_START:
				parseStart(b, r)
			case _SM_UNITL_CRLF:
				parseUnitlCrlf(b, r)
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

func parseUnitlCrlf(b byte, rc *RedisCommand) {
	rc.value.appendToBuffer(b)
	if rc.value.bufferEndByCtrf() {
		var err error = nil
		if rc._type == RC_TYPE_STATUS || rc._type == RC_TYPE_ERROR {
			err = rc.value.bufferToBytes()
			rc.status = _SM_END
		} else if rc._type == RC_TYPE_INT {
			err = rc.value.bufferToInt()
			rc.status = _SM_END
		}

		if err != nil {
			rc.status = _SM_ERROR
		}
	}
}
