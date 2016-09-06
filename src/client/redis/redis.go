package redis

import (
	"net"
	"time"
)

type Conn struct {
	conn net.Conn
}

func NewConn(addr string, timeout time.Duration) (*Conn, error) {
	conn, err := net.DialTimeout("tcp", addr, timeout)
	if err != nil {
		return nil, err
	}
	return &Conn{
		conn: conn,
	}, nil
}
