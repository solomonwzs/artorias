package redis

import "net"

type RedisDialer struct {
	addr string
}

func NewRedisDialer(addr string) *RedisDialer {
	return &RedisDialer{
		addr: addr,
	}
}

func (rd *RedisDialer) NewConn() (net.Conn, error) {
	conn, err := net.Dial("tcp", rd.addr)
	return conn, err
}

func (rd *RedisDialer) DelConn(conn net.Conn) {
	conn.Close()
}
