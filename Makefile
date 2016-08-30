# vim: noet:

BASE = ${PWD}
GOPATH := ${BASE}:${GOPATH}
BUILD_PATH = "bin"

all:
	@[ -d $(dir $(BUILD_PATH)) ] || mkdir $(BUILD_PATH) 2>/dev/null
	@go build -o $(BUILD_PATH)/main.goc src/main.go
