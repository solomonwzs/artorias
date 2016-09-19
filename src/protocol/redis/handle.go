package redis

import (
	dialRedis "dial/redis"
	"logger"
	"redisparser"
	"socketframe/server"
)

type Redis struct {
	redisConnPools []*dialRedis.RedisConnPool
}

func (*Redis) Init(worker *server.Worker) *server.ProtocolInitState {
	return server.InitStateOK(newConnState())
}

func (*Redis) HandleBytes(buf []byte, state0 interface{}) *server.ProtocolReply {
	//return server.ReplyClient([]byte("-ERROR\r\n"), state0)
	logger.Logf(logger.DEBUG, "%#v\n", string(buf))
	state := state0.(*connState)
	state.buf = append(state.buf, buf...)

	i := state.parser.Write(state.buf)
	state.buf = state.buf[i:]

	cmd := state.parser.GetCommand()
	if cmd == nil {
		return server.ReplyNoReply(state)
	}

	state.parser = redisparser.NewParser()
	if cmd.OK() {
		replyBytes := processCommand(cmd)
		return server.ReplyClient(replyBytes, state)
	} else {
		return server.ReplyClient([]byte("-ERROR\r\n"), state)
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
