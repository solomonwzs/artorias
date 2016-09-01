package redis

const (
	_SM_ERROR int = iota
	_SM_START
	_SM_PARSE_ARGN
	_SM_PARSE_ARGN_END
	_SM_PARSE_ARGN_OK
	_SM_PARSE_ARG_LEN
	_SM_PARSE_ARG_LEN_END
	_SM_PARSE_ARG
	_SM_PARSE_ARG_END
	_SM_END
)

type command struct {
	argc         int
	argv         [][]byte
	stateMachine int
	buf          []byte
	argvLen      int

	pargc    int
	pargvLen int
}

func newCommand() *command {
	return &command{
		argc:         0,
		argv:         nil,
		stateMachine: _SM_START,
		buf:          []byte{},
		argvLen:      0,

		pargc:    0,
		pargvLen: 0,
	}
}

func (cmd *command) smStart(char byte) {
	if char == '*' {
		cmd.stateMachine = _SM_PARSE_ARGN
	}
}

func (cmd *command) smParseArgn(char byte) {
	if char >= '0' && char <= '9' {
		cmd.buf = append(cmd.buf, char)
	} else if char == '\r' && len(cmd.buf) != 0 {
		cmd.stateMachine = _SM_PARSE_ARGN_END
	} else {
		cmd.stateMachine = _SM_ERROR
	}
}

func (cmd *command) smParseArgnEnd(char byte) {
	if char == '\n' {
		cmd.argc = 0
		for _, char := range cmd.buf {
			cmd.argc = cmd.argc*10 + int(char-'0')
		}
		cmd.argv = make([][]byte, cmd.argc)
		cmd.buf = []byte{}
		cmd.stateMachine = _SM_PARSE_ARGN_OK
	} else {
		cmd.stateMachine = _SM_ERROR
	}
}

func (cmd *command) smParseArgnOK(char byte) {
	if char == '$' {
		cmd.stateMachine = _SM_PARSE_ARG_LEN
	} else {
		cmd.stateMachine = _SM_ERROR
	}
}

func (cmd *command) smParseArgLen(char byte) {
	if char >= '0' && char <= '9' {
		cmd.buf = append(cmd.buf, char)
	} else if char == '\r' && len(cmd.buf) != 0 {
		cmd.stateMachine = _SM_PARSE_ARG_LEN_END
	} else {
		cmd.stateMachine = _SM_ERROR
	}
}

func (cmd *command) smParseArgLenEnd(char byte) {
	if char == '\n' {
		cmd.argvLen = 0
		for _, ch := range cmd.buf {
			cmd.argvLen = cmd.argvLen*10 + int(ch-'0')
		}
		cmd.buf = []byte{}
		cmd.argv[cmd.pargc] = make([]byte, cmd.argvLen)
		cmd.pargvLen = 0
		cmd.stateMachine = _SM_PARSE_ARG
	} else {
		cmd.stateMachine = _SM_ERROR
	}
}

func (cmd *command) smParseArg(char byte) {
	if cmd.pargvLen < cmd.argvLen {
		cmd.argv[cmd.pargc][cmd.pargvLen] = char
		cmd.pargvLen++
	} else if cmd.pargvLen == cmd.argvLen && char == '\r' {
		cmd.stateMachine = _SM_PARSE_ARG_END
	} else {
		cmd.stateMachine = _SM_ERROR
	}
}

func (cmd *command) smParseArgEnd(char byte) {
	if char == '\n' {
		cmd.pargc++
		if cmd.pargc == cmd.argc {
			cmd.stateMachine = _SM_END
		} else {
			cmd.stateMachine = _SM_PARSE_ARGN_OK
		}
	} else {
		cmd.stateMachine = _SM_ERROR
	}
}

func (cmd *command) parseCommand(input []byte) int {
	for i, char := range input {
		switch cmd.stateMachine {
		case _SM_START:
			cmd.smStart(char)
		case _SM_PARSE_ARGN:
			cmd.smParseArgn(char)
		case _SM_PARSE_ARGN_END:
			cmd.smParseArgnEnd(char)
		case _SM_PARSE_ARGN_OK:
			cmd.smParseArgnOK(char)
		case _SM_PARSE_ARG_LEN:
			cmd.smParseArgLen(char)
		case _SM_PARSE_ARG_LEN_END:
			cmd.smParseArgLenEnd(char)
		case _SM_PARSE_ARG:
			cmd.smParseArg(char)
		case _SM_PARSE_ARG_END:
			cmd.smParseArgEnd(char)
		}

		if cmd.stateMachine == _SM_END || cmd.stateMachine == _SM_ERROR {
			return i
		}
	}
	return len(input) - 1
}

type connState struct {
	buf []byte
	cmd *command
}

func newConnState() *connState {
	return &connState{
		buf: []byte{},
		cmd: newCommand(),
	}
}
