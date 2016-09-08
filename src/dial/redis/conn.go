package redis

import (
	"io"
	"logger"
	"net"
	"time"
)

type RedisConn struct {
	conn net.Conn
	ver  time.Time
}

func (rc *RedisConn) Query(argn int, argv [][]byte) {
	cmd := wrapCommand(argn, argv)
	rc.conn.Write(cmd)

	for {
		buf := make([]byte, 1024)
		n, err := rc.conn.Read(buf)

		if err == io.EOF {
			logger.Logf(logger.DEBUG, "%#v\n", string(buf[:n]))
			break
		} else if err != nil {
			logger.Log(logger.ERROR, err)
			break
		}
		logger.Logf(logger.DEBUG, "%#v\n", string(buf[:n]))
	}
}
