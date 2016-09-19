package redis

import "fmt"

func WrapMulitBulk(argn int, argv [][]byte) []byte {
	wc := []byte(fmt.Sprintf("*%d\r\n", argn))
	for i := 0; i < argn; i++ {
		wc = append(wc, WrapBulk(argv[i])...)
	}
	return wc
}

func WrapBulk(bs []byte) []byte {
	if bs == nil {
		return []byte("$-1\r\n")
	}
	return []byte(fmt.Sprintf("$%d\r\n%s\r\n", len(bs), bs))
}
