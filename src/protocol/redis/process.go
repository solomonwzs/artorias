package redis

import (
	dialRedis "dial/redis"
	"redisparser"
	"strings"
)

var _IGNORE_COMMAND = []string{
	"COMMAND",
	"SELECT",
}

var store = map[string][]byte{}

func processCommand(cmd *redisparser.RedisCommand) []byte {
	if cmd.Type() == redisparser.RC_TYPE_MULIT {
		argv := cmd.Commands()
		if len(argv) > 0 {
			cmdStr := strings.ToUpper(string(argv[0].Bytes()))

			for _, s := range _IGNORE_COMMAND {
				if cmdStr == s {
					return []byte("+OK\r\n")
				}
			}

			if cmdStr == "GET" && len(argv) == 2 {
				key := string(argv[1].Bytes())
				val, exist := store[key]
				if exist {
					return dialRedis.WrapBulk(val)
				} else {
					return []byte("$-1\r\n")
				}
			}
			// if cmdStr == "SET" && len(argv) == 3 {
			// 	key := string(argv[1].Bytes())
			// 	store[key] = argv[2].Bytes()
			// }
			if cmdStr == "PING" {
				if len(argv) == 1 {
					return []byte("$4\r\nPONG\r\n")
				} else if len(argv) == 2 {
					return dialRedis.WrapBulk(argv[1].Bytes())
				}
			}
		}
	}
	return []byte("+OK\r\n")
}
