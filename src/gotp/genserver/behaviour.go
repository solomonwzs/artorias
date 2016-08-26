package genserver

import (
	"gotp"
	"time"
)

type Options struct {
}

type InitResult struct {
	Status     gotp.KeyAtom
	State      interface{}
	StopReason interface{}
	Timeout    time.Duration
}

type CallResult struct {
	Status     gotp.KeyAtom
	Replay     interface{}
	NewState   interface{}
	StopReason interface{}
	Timeout    time.Duration
}

type GenServerModule interface {
	Init(args []interface{}) (result *InitResult)
	Terminate(reason interface{}, state interface{})

	HandleCall(request interface{}, from *gotp.MessageChannel, state interface{})
	HandleCast(request interface{}, state interface{})
}

func OkInitResult(state interface{}) *InitResult {
	return &InitResult{
		Status: gotp.ATOM_OK,
		State:  state,
	}
}

func IgnoreInitResult() *InitResult {
	return &InitResult{
		Status: gotp.ATOM_IGNORE,
	}
}

func StopInitResult(stopReason interface{}) *InitResult {
	return &InitResult{
		Status:     gotp.ATOM_STOP,
		StopReason: stopReason,
	}
}

func ReplyCallResult(replay interface{}, newState interface{}) *CallResult {
	return &CallResult{
		Status:   gotp.ATOM_REPLY,
		Replay:   replay,
		NewState: newState,
	}
}

func StopCallResult(stopReason interface{}, replay interface{}, newState interface{}) *CallResult {
	return &CallResult{
		Status:     gotp.ATOM_STOP,
		StopReason: stopReason,
		Replay:     replay,
		NewState:   newState,
	}
}
