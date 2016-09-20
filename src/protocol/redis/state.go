package redis

import (
	"redisparser"
	"sync"
)

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

type storeBlock struct {
	v   map[string][]byte
	mux sync.Mutex
}

func newStoreBlock() *storeBlock {
	return &storeBlock{
		v: map[string][]byte{},
	}
}

func (sb *storeBlock) get(key string) (value []byte, exist bool) {
	sb.mux.Lock()
	defer sb.mux.Unlock()

	value, exist = sb.v[key]
	return
}

func (sb *storeBlock) set(key string, value []byte) {
	sb.mux.Lock()
	sb.v[key] = value
	sb.mux.Unlock()
}
