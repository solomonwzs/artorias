package redis

import "strings"

var _IGNORE_COMMAND = []string{
	"COMMAND",
	"SELECT",
}

func isIgnoreCommand(cmd *command) bool {
	if cmd.argc > 0 {
		cmdStr := strings.ToUpper(string(cmd.argv[0]))
		for _, s := range _IGNORE_COMMAND {
			if cmdStr == s {
				return true
			}
		}
	}
	return false
}
