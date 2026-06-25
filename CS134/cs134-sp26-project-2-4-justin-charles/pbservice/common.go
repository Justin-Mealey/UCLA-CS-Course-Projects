package pbservice

const (
	OK             = "OK"
	ErrNoKey       = "ErrNoKey"
	ErrWrongServer = "ErrWrongServer"
)

type Err string

type PutAppendArgs struct {
	Key      string
	Value    string
	Op       string
	Id       int64
	ClientId int64
}

type PutAppendReply struct {
	Err Err
}

type GetArgs struct {
	Key      string
	Id       int64
	ClientId int64
}

type GetReply struct {
	Err   Err
	Value string
}

type ForwardGetArgs struct {
	Key      string
	Id       int64
	ClientId int64
}

type ForwardGetReply struct {
	Err Err
}

type ForwardPutAppendArgs struct {
	Key      string
	Value    string
	Op       string
	Id       int64
	ClientId int64
}

type ForwardPutAppendReply struct {
	Err Err
}

type TransferArgs struct {
	DB  map[string]string
	Dup map[int64]string
}

type TransferReply struct {
	Err Err
}
