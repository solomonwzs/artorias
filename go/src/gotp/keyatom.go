package gotp

type KeyAtom uint

var (
	ATOM_OK        = KeyAtom(0x0000)
	ATOM_STOP      = KeyAtom(0x0001)
	ATOM_HIBERNATE = KeyAtom(0x0002)
	ATOM_IGNORE    = KeyAtom(0x0003)
	ATOM_REPLY     = KeyAtom(0x0004)
	ATOM_NOREPLY   = KeyAtom(0x0005)
)
