package genserver

import "gotp"

type Options struct {
}

type InitResult struct {
	Status gotp.KeyAtom
}

type GenServerModule interface {
	Init(args []interface{}) (state interface{}, err error)
	Terminate(reason interface{}, state interface{})

	HandleCall(request interface{}, from *gotp.MessageChannel, state interface{})
	HandleCast(request interface{}, state interface{})
}

func Start(module GenServerModule, args []interface{}, options *Options) (
	pid *gotp.Pid, err error) {
	return nil, nil
}
