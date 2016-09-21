# vim: noet:

BASE = ${PWD}
GOPATH := ${BASE}:${GOPATH}
BUILD_PATH = "bin"

all:
	@mkdir -p $(BUILD_PATH)
	@go build -o $(BUILD_PATH)/main.goc src/main.go
