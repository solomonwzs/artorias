package redis

import (
	"errors"
	"fmt"
	"net"
	"socketframe/client"
	"strings"
	"time"
)

const (
	_REDIS_RES_OK = "+OK\r\n"
)

var (
	ERR_INIT_CONN_REDIS_FAIL = errors.New("init redis conn fail")
)

type redisDialer struct {
	addr string
	db   string
}

func (rd *redisDialer) NewConn() (net.Conn, error) {
	conn, err := net.Dial("tcp", rd.addr)
	if err != nil {
		return nil, err
	}

	_, err = conn.Write(wrapCommand(2, [][]byte{
		[]byte("SELECT"),
		[]byte(rd.db),
	}))
	if err != nil {
		conn.Close()
		return nil, ERR_INIT_CONN_REDIS_FAIL
	}

	buf := make([]byte, 8)
	n, err := conn.Read(buf)
	if err != nil || string(buf[:n]) != _REDIS_RES_OK {
		conn.Close()
		return nil, ERR_INIT_CONN_REDIS_FAIL
	}

	return conn, nil
}

func (rd *redisDialer) DelConn(conn net.Conn) {
	conn.Close()
}

type RedisConnPool struct {
	pool   *client.Pool
	dialer *redisDialer
	size   int
	ver    time.Time
}

func NewRedisConnPool(conf string, size int) *RedisConnPool {
	arr := strings.Split(conf, ":")
	addr := fmt.Sprintf("%s:%s", arr[0], arr[1])
	db := arr[2]
	if db == "" {
		db = "0"
	}
	dialer := &redisDialer{
		addr: addr,
		db:   db,
	}
	pool := client.NewPool(dialer, size)
	return &RedisConnPool{
		size:   size,
		dialer: dialer,
		pool:   pool,
		ver:    time.Now(),
	}
}

func (rcp *RedisConnPool) GetConn() (*RedisConn, error) {
	conn, err := rcp.pool.GetConn()
	if err != nil {
		return nil, err
	}
	return &RedisConn{
		conn: conn,
		ver:  rcp.ver,
	}, nil
}

func (rcp *RedisConnPool) RecoverConn(rc *RedisConn) {
	rcp.pool.RecoverConn(rc.conn)
}
