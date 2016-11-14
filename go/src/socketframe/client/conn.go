package client

import "net"

type ConnDialer interface {
	NewConn() (net.Conn, error)
	DelConn(net.Conn)
}
