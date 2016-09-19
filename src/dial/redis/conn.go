package redis

import (
	"io"
	"logger"
	"net"
	"redisparser"
	"time"
)

type RedisConn struct {
	conn net.Conn
	ver  time.Time
}

func (rc *RedisConn) Query(argn int, argv [][]byte) {
	cmd := WrapMulitBulk(argn, argv)
	rc.conn.Write(cmd)

	p := redisparser.NewParser()
	buf := make([]byte, 1024)
	for {
		n, err := rc.conn.Read(buf)

		if err == io.EOF {
			logger.Logf(logger.DEBUG, "%#v\n", string(buf[:n]))
			break
		} else if err != nil {
			logger.Log(logger.ERROR, err)
			break
		}
		logger.Logf(logger.DEBUG, "%#v\n", string(buf[:n]))
		p.Write(buf[:n])

		cmd := p.GetCommand()
		if cmd != nil {
			logger.Log(logger.DEBUG, cmd)
			break
		}
	}
}
