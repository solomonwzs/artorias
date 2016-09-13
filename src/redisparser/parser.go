package redisparser

import (
	"logger"
	"strconv"
)

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

func (rcv *redisCommandValue) toInt() error {
	i, err := strconv.Atoi(string(rcv._Bytes))
	if err != nil {
		return err
	} else {
		rcv._Int = i
		rcv._Bytes = nil
		return nil
	}
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

// func (rc *RedisCommand) Parser() {
// 	byteChannel := make(chan byte)
//
// 	go func() {
// 		for _ := range byteChannel {
// 		}
// 	}()
// }

func parse(rc *RedisCommand, byteChannel chan byte) {
	b := <-byteChannel
	switch b {
	case '+':
		rc._type = RC_TYPE_STATUS
		rc.status = _SM_UNITL_CRLF
		rc.value._Bytes = []byte{}
	case '-':
		rc._type = RC_TYPE_ERROR
		rc.status = _SM_UNITL_CRLF
		rc.value._Bytes = []byte{}
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

func parserStart(rc *RedisCommand, b byte) {
	switch b {
	case '+':
		rc._type = RC_TYPE_STATUS
		rc.status = _SM_UNITL_CRLF
		rc.value._Bytes = []byte{}
	case '-':
		rc._type = RC_TYPE_ERROR
		rc.status = _SM_UNITL_CRLF
		rc.value._Bytes = []byte{}
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

func parserUnitlCrlf(rc *RedisCommand, b byte) {
	if b == '\r' {
		rc.status = _SM_UNITL_CRLF_END
	} else {
		rc.value._Bytes = append(rc.value._Bytes, b)
	}
	switch b {
	case '\r':
		rc.status = _SM_UNITL_CRLF_END
	default:
		rc.value._Bytes = append(rc.value._Bytes, b)
	}
}

func parserUnitlCrlfEnd(rc *RedisCommand, b byte) {
	switch b {
	case '\n':
		rc.status = _SM_UNITL_CRLF_OK
		if rc._type == RC_TYPE_INT || rc._type == RC_TYPE_MULIT {
			err := rc.value.toInt()
			if err != nil {
				logger.Log(logger.ERROR, err)
				rc.status = _SM_ERROR
				return
			}
		}
		if rc._type != RC_TYPE_MULIT {
			rc._type = _SM_END
			return
		} else {
		}
	case '\r':
		rc.value._Bytes = append(rc.value._Bytes, '\r')
	default:
		rc.status = _SM_UNITL_CRLF
		rc.value._Bytes = append(rc.value._Bytes, '\r', b)
	}
}
