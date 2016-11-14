package gotp

type Message struct {
	content interface{}
}

type Pid struct {
	acceptor chan Message
}

func PackMessage(i interface{}) Message {
	return Message{
		content: i,
	}
}

func (msg Message) UnPackMessage() interface{} {
	return msg.content
}

func (msg Message) SendTo(pid *Pid) {
	pid.acceptor <- msg
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
