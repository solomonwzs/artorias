package redis

import (
	"logger"
	"socketframe"
)

type Redis struct{}

func (*Redis) Init() interface{} {
	return newConnState()
}

func (*Redis) Handle(buf []byte, state0 interface{}) *socketframe.ProtocolReplay {
	logger.Logf(logger.DEBUG, "%#v\n", string(buf))
	state := state0.(*connState)
	state.buf = append(state.buf, buf...)

	i := state.cmd.parseCommand(state.buf)
	logger.Logf(logger.DEBUG, "%+v\n", state.cmd)
	if state.cmd.stateMachine == _SM_END || state.cmd.stateMachine == _SM_ERROR {
		state.cmd = newCommand()
	}
	state.buf = state.buf[i+1:]

	return &socketframe.ProtocolReplay{
		Replay:   false,
		NewState: state,
	}
}
