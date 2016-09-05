package redis

import (
	"logger"
	"socketframe"
)

type Redis struct{}

func (*Redis) Init(worker *socketframe.Worker) *socketframe.ProtocolInitState {
	return socketframe.InitStateOK(newConnState())
}

func (*Redis) HandleBytes(buf []byte, state0 interface{}) *socketframe.ProtocolReply {
	logger.Logf(logger.DEBUG, "%#v\n", string(buf))
	state := state0.(*connState)
	state.buf = append(state.buf, buf...)

	i := state.cmd.parseCommand(state.buf)
	state.buf = state.buf[i+1:]

	logger.Logf(logger.DEBUG, "%+v\n", state.cmd)
	switch state.cmd.stateMachine {
	case _SM_END:
		replyBytes := processCommand(state.cmd)
		state.cmd = newCommand()
		return socketframe.ReplyClient(replyBytes, state)
	case _SM_ERROR:
		state.cmd = newCommand()
		return socketframe.ReplyNoReply(state)
	default:
		return socketframe.ReplyNoReply(state)
	}
}

func (*Redis) HandleInfo(info interface{}, state0 interface{}) *socketframe.ProtocolReply {
	logger.Log(logger.DEBUG, info)
	state := state0.(*connState)

	return socketframe.ReplyNoReply(state)
}

func (*Redis) Terminal(reason int, err interface{}, state0 interface{}) {
	logger.Log(logger.DEBUG, reason, err)
}

func processCommand(cmd *command) []byte {
	return []byte("+\r\n")
}
