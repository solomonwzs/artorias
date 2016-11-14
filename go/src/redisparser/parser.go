package redisparser

import (
	"fmt"
	"strconv"
	"strings"
)

const (
	_SM_START = iota
	_SM_UNITL_CRLF
	_SM_FIXED_LEN
	_SM_MULIT
	_SM_END
	_SM_ERROR
)

var _MSG_SM map[int]string = map[int]string{
	_SM_START:      "start",
	_SM_UNITL_CRLF: "until_CRLF",
	_SM_FIXED_LEN:  "fixed_len",
	_SM_MULIT:      "mulit",
	_SM_END:        "end",
	_SM_ERROR:      "error",
}

const (
	RC_TYPE_STATUS = iota
	RC_TYPE_ERROR
	RC_TYPE_BULK
	RC_TYPE_INT
	RC_TYPE_MULIT
	RC_TYPE_UNKNOWN
)

var _MSG_CMD_TYPE map[int]string = map[int]string{
	RC_TYPE_STATUS:  "status",
	RC_TYPE_ERROR:   "error",
	RC_TYPE_BULK:    "bulk",
	RC_TYPE_INT:     "int",
	RC_TYPE_MULIT:   "mulit",
	RC_TYPE_UNKNOWN: "unknown",
}

func endByCtrf(buffer []byte) bool {
	l := len(buffer)
	return l >= 2 && buffer[l-2] == '\r' && buffer[l-1] == '\n'
}

type redisCommandValue struct {
	_Int      int
	_Bytes    []byte
	_Commands []*RedisCommand
}

func (rcv *redisCommandValue) String() string {
	vals := []string{}
	for _, c := range rcv._Commands {
		vals = append(vals, fmt.Sprintf("%v", c))
	}
	return fmt.Sprintf("{\"int\":%d,\"bytes\":%q,\"commands\":[%s]}",
		rcv._Int, rcv._Bytes, strings.Join(vals, ","))
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

func newRedisCommandValue() *redisCommandValue {
	return &redisCommandValue{
		_Int:      0,
		_Bytes:    nil,
		_Commands: nil,
	}
}

type RedisCommand struct {
	_type  int
	value  *redisCommandValue
	status int
}

func newRedisCommand() *RedisCommand {
	rc := &RedisCommand{
		_type:  RC_TYPE_UNKNOWN,
		value:  newRedisCommandValue(),
		status: _SM_START,
	}

	return rc
}

func (rc *RedisCommand) String() string {
	return fmt.Sprintf("{\"type\":%q,\"status\":%q,\"value\":%v}",
		_MSG_CMD_TYPE[rc._type], _MSG_SM[rc.status], rc.value)
}

func (rc *RedisCommand) OK() bool {
	return rc._type != RC_TYPE_UNKNOWN && rc.status == _SM_END
}

func (rc *RedisCommand) Type() int {
	return rc._type
}

func (rc *RedisCommand) Int() int {
	return rc.value._Int
}

func (rc *RedisCommand) Bytes() []byte {
	return rc.value._Bytes
}

func (rc *RedisCommand) Commands() []*RedisCommand {
	return rc.value._Commands
}

type Parser struct {
	stack        []*RedisCommand
	buffer       []byte
	argLen       int
	redisCommand *RedisCommand
}

func NewParser() *Parser {
	rc := newRedisCommand()
	return &Parser{
		stack:        []*RedisCommand{rc},
		buffer:       []byte{},
		argLen:       0,
		redisCommand: rc,
	}
}

func (ps *Parser) Write(bs []byte) int {
	if ps.stack == nil || len(ps.stack) == 0 {
		return 0
	}

	for i, b := range bs {
		r := ps.stack[len(ps.stack)-1]

		switch r.status {
		case _SM_START:
			parseStart(ps, b, r)
		case _SM_UNITL_CRLF:
			parseUnitlCrlf(ps, b, r)
		case _SM_FIXED_LEN:
			parseFixedLen(ps, b, r)
		}

		if len(ps.stack) == 0 {
			return i + 1
		}
		if r.status == _SM_ERROR {
			ps.stack = nil
			ps.redisCommand.status = _SM_ERROR
			return i + 1
		}
	}
	return len(bs)
}

func (ps *Parser) GetCommand() *RedisCommand {
	if ps.stack == nil || len(ps.stack) == 0 {
		return ps.redisCommand
	} else {
		return nil
	}
}

func (ps *Parser) popStacks() {
	l := len(ps.stack)
	for l > 0 {
		top := ps.stack[l-1]
		if top.status == _SM_END {
			ps.stack = ps.stack[:l-1]
			l -= 1
		} else if top.status == _SM_MULIT {
			top.status = _SM_END
			ps.stack = ps.stack[:l-1]
			l -= 1
		} else {
			return
		}
	}
}

func parseFixedLen(ps *Parser, b byte, rc *RedisCommand) {
	ps.buffer = append(ps.buffer, b)
	l := len(ps.buffer)
	if l == ps.argLen+2 {
		if endByCtrf(ps.buffer) {
			rc.value.setBytes(ps.buffer[:l-2])
			rc.status = _SM_END
			ps.popStacks()
		} else {
			rc.status = _SM_ERROR
		}
	}
}

func parseStart(ps *Parser, b byte, rc *RedisCommand) {
	switch b {
	case '+':
		rc._type = RC_TYPE_STATUS
	case '-':
		rc._type = RC_TYPE_ERROR
	case '$':
		rc._type = RC_TYPE_BULK
	case ':':
		rc._type = RC_TYPE_INT
	case '*':
		rc._type = RC_TYPE_MULIT
	default:
		rc._type = RC_TYPE_UNKNOWN
		rc.status = _SM_ERROR
	}
	ps.buffer = []byte{}
	rc.status = _SM_UNITL_CRLF
}

func parseUnitlCrlf(ps *Parser, b byte, rc *RedisCommand) {
	ps.buffer = append(ps.buffer, b)
	if endByCtrf(ps.buffer) {
		var err error = nil
		blen := len(ps.buffer)
		if rc._type == RC_TYPE_STATUS || rc._type == RC_TYPE_ERROR ||
			rc._type == RC_TYPE_UNKNOWN {

			rc.value.setBytes(ps.buffer[:blen-2])
			rc.status = _SM_END
			ps.popStacks()
		} else if rc._type == RC_TYPE_INT {
			i, err := strconv.Atoi(string(ps.buffer[:blen-2]))
			if err == nil {
				rc.value.setInt(i)
				rc.status = _SM_END
				ps.popStacks()
			}
		} else if rc._type == RC_TYPE_BULK {
			i, err := strconv.Atoi(string(ps.buffer[:blen-2]))
			if err == nil {
				if i < 0 {
					rc.value.setBytes(nil)
					rc.status = _SM_END
					ps.popStacks()
				} else {
					ps.argLen = i
					rc.status = _SM_FIXED_LEN
				}
			}
		} else if rc._type == RC_TYPE_MULIT {
			i, err := strconv.Atoi(string(ps.buffer[:blen-2]))
			if err == nil {
				rc.status = _SM_MULIT
				rcs := make([]*RedisCommand, i)
				for j := 0; j < i; j++ {
					rcs[j] = newRedisCommand()
				}
				rc.value.setCommands(rcs)
				for j := i - 1; j >= 0; j-- {
					ps.stack = append(ps.stack, rcs[j])
				}
			}
		}

		ps.buffer = []byte{}
		if err != nil {
			rc.status = _SM_ERROR
		}
	}
}
