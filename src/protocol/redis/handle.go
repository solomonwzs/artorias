package redis

import (
	"logger"
	"socketframe"
)

type Redis struct{}

func (*Redis) Init() interface{} {
	return &connState{
		buf: []byte{},
	}
}

func (*Redis) Handle(buf []byte, state0 interface{}) *socketframe.ProtocolReplay {
	logger.Logf(logger.DEBUG, "%#v\n", string(buf))
	state := state0.(*connState)
	state.buf = append(state.buf, buf...)

	i := 0
	for i < 10 {
		r := state.getToken()
		if r == nil {
			break
		}
		logger.Log(logger.DEBUG, string(r))
		i++
	}

	return &socketframe.ProtocolReplay{
		Replay:   false,
		NewState: state,
	}
}
