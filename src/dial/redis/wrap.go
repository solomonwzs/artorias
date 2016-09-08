package redis

import "fmt"

func wrapCommand(argn int, argv [][]byte) []byte {
	wc := []byte(fmt.Sprintf("*%d\r\n", argn))
	for i := 0; i < argn; i++ {
		wc = append(wc, []byte(fmt.Sprintf("$%d\r\n%s\r\n",
			len(argv[i]), argv[i]))...)
	}
	return wc
}
