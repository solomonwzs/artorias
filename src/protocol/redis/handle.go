package redis

import (
	"logger"
	"socketframe"
)

type Redis struct{}

func (*Redis) Init() interface{} {
	return newConnState()
}

func (*Redis) HandleBytes(buf []byte, state0 interface{}) *socketframe.ProtocolReplay {
	logger.Logf(logger.DEBUG, "%#v\n", string(buf))
	state := state0.(*connState)
	state.buf = append(state.buf, buf...)

	i := state.cmd.parseCommand(state.buf)
	state.buf = state.buf[i+1:]

	logger.Logf(logger.DEBUG, "%+v\n", state.cmd)
	if state.cmd.stateMachine == _SM_END || state.cmd.stateMachine == _SM_ERROR {
		state.cmd = newCommand()
		return &socketframe.ProtocolReplay{
			Replay:   true,
			Result:   []byte("hello"),
			NewState: state,
		}
	}

	return &socketframe.ProtocolReplay{
		Replay:   false,
		NewState: state,
	}
}

func (*Redis) HandleInfo(info interface{}, state0 interface{}) *socketframe.ProtocolReplay {
	logger.Log(logger.DEBUG, info)
	state := state0.(*connState)

	return &socketframe.ProtocolReplay{
		Replay:   false,
		NewState: state,
	}
}

func (*Redis) Terminal(reason int, state0 interface{}) {
}
