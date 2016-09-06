package client

import (
	"logger"
	"net"
)

const (
	_PM_GET = iota
	_PM_PUT
)

type poolMsgIn struct {
	action int
	conn   net.Conn
	out    chan *poolMsgOut
}

type poolMsgOut struct {
	conn net.Conn
	err  error
}

type Pool struct {
	size       int
	connPool   []net.Conn
	connDialer ConnDialer
	msgChannel chan *poolMsgIn
}

func loop(pool *Pool) {
	for msg := range pool.msgChannel {
		switch msg.action {
		case _PM_GET:
			if len(pool.connPool) == 0 {
				logger.Log(logger.DEBUG, "pool is empty, gen new conn")
				conn, err := pool.connDialer.NewConn()
				msg.out <- &poolMsgOut{
					conn: conn,
					err:  err,
				}
			} else {
				logger.Log(logger.DEBUG, "get conn from pool")
				conn := pool.connPool[0]
				pool.connPool = pool.connPool[1:]
				msg.out <- &poolMsgOut{
					conn: conn,
					err:  nil,
				}
			}
		case _PM_PUT:
			if len(pool.connPool) >= pool.size {
				logger.Log(logger.DEBUG, "pool is full, close conn")
				pool.connDialer.DelConn(msg.conn)
			} else {
				logger.Log(logger.DEBUG, "recover conn")
				pool.connPool = append(pool.connPool, msg.conn)
			}
		}
	}
}

func NewPool(connDialer ConnDialer, size int) *Pool {
	pool := &Pool{
		size:       size,
		connPool:   []net.Conn{},
		connDialer: connDialer,
		msgChannel: make(chan *poolMsgIn),
	}
	go func() {
		loop(pool)
	}()
	return pool
}

func (pool *Pool) GetConn() (net.Conn, error) {
	out := make(chan *poolMsgOut)
	pool.msgChannel <- &poolMsgIn{
		action: _PM_GET,
		out:    out,
	}
	res := <-out
	return res.conn, res.err
}

func (pool *Pool) RecoverConn(conn net.Conn) {
	pool.msgChannel <- &poolMsgIn{
		action: _PM_PUT,
		conn:   conn,
	}
}
