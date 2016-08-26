package gotp

type Message struct {
	content interface{}
}

type MessageChannel struct {
	channel chan Message
}

type Pid struct {
	acceptor MessageChannel
}
