package gotp

type Message struct {
	content interface{}
}

type Pid struct {
	acceptor chan Message
}

func (pid *Pid) Recv(message Message) {
	pid.acceptor <- message
}

func Run(do func(Message)) *Pid {
	p := &Pid{
		acceptor: make(chan Message, 16),
	}
	go func() {
		for message := range p.acceptor {
			do(message)
		}
	}()
	return p
}
