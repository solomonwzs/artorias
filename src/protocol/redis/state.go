package redis

import "redisparser"

type connState struct {
	buf    []byte
	parser *redisparser.Parser
}

func newConnState() *connState {
	return &connState{
		buf:    []byte{},
		parser: redisparser.NewParser(),
	}
}
