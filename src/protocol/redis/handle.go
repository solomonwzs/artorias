package redis

import (
	dialRedis "dial/redis"

	"logger"
	"socketframe/server"
)

type Redis struct {
	redisConnPools []*dialRedis.RedisConnPool
}

func (*Redis) Init(worker *server.Worker) *server.ProtocolInitState {
	return server.InitStateOK(newConnState())
}

func (*Redis) HandleBytes(buf []byte, state0 interface{}) *server.ProtocolReply {
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
		return server.ReplyClient(replyBytes, state)
	case _SM_ERROR:
		state.cmd = newCommand()
		return server.ReplyNoReply(state)
	default:
		return server.ReplyNoReply(state)
	}
}

func (*Redis) HandleInfo(info interface{}, state0 interface{}) *server.ProtocolReply {
	logger.Log(logger.DEBUG, info)
	state := state0.(*connState)

	return server.ReplyNoReply(state)
}

func (*Redis) Terminal(reason int, err interface{}, state0 interface{}) {
	logger.Log(logger.DEBUG, reason, err)
}

func processCommand(cmd *command) []byte {
	if isIgnoreCommand(cmd) {
		return []byte("+OK\r\n")
	}
	return []byte("+OK\r\n")
}
