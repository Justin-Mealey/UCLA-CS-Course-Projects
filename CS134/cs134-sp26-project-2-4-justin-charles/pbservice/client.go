package pbservice

import "cs134-kv/viewservice"
import "net/rpc"
import "fmt"
import "time"

import "crypto/rand"
import "math/big"

type Clerk struct {
	vs      *viewservice.Clerk
	me      int64
	primary string
}

// this may come in handy.
func nrand() int64 {
	max := big.NewInt(int64(1) << 62)
	bigx, _ := rand.Int(rand.Reader, max)
	x := bigx.Int64()
	return x
}

func MakeClerk(vshost string, me string) *Clerk {
	ck := new(Clerk)
	ck.vs = viewservice.MakeClerk(me, vshost)
	ck.me = nrand()

	return ck
}

// call() sends an RPC to the rpcname handler on server srv
// with arguments args, waits for the reply, and leaves the
// reply in reply. the reply argument should be a pointer
// to a reply structure.
//
// the return value is true if the server responded, and false
// if call() was not able to contact the server. in particular,
// the reply's contents are only valid if call() returned true.
//
// you should assume that call() will return an
// error after a while if the server is dead.
// don't provide your own time-out mechanism.
//
// please use call() to send all RPCs, in client.go and server.go.
// please don't change this function except to comment out the fmt.Println line.
func call(srv string, rpcname string,
	args interface{}, reply interface{}) bool {
	c, errx := rpc.Dial("unix", srv)
	if errx != nil {
		return false
	}
	defer c.Close()

	err := c.Call(rpcname, args, reply)
	if err == nil {
		return true
	}

	// Feel free to comment this line out
	fmt.Println("pbservice/client.go/call():", err)
	return false
}

func (ck *Clerk) Get(key string) string {
	args := &GetArgs{
		Key:      key,
		Id:       nrand(),
		ClientId: ck.me,
	}

	for {
		if ck.primary == "" {
			view, _ := ck.vs.Get()
			ck.primary = view.Primary
			time.Sleep(viewservice.PingInterval)
			continue
		}

		var reply GetReply
		ok := call(ck.primary, "PBServer.Get", args, &reply)
		if ok {
			if reply.Err == OK || reply.Err == ErrNoKey {
				return reply.Value
			}
		}

		ck.primary = ""
		time.Sleep(viewservice.PingInterval)
	}
}

func (ck *Clerk) PutAppend(key string, value string, op string) {
	args := &PutAppendArgs{
		Key:      key,
		Value:    value,
		Op:       op,
		Id:       nrand(),
		ClientId: ck.me,
	}

	for {
		if ck.primary == "" {
			view, _ := ck.vs.Get()
			ck.primary = view.Primary
			time.Sleep(viewservice.PingInterval)
			continue
		}

		var reply PutAppendReply
		ok := call(ck.primary, "PBServer.PutAppend", args, &reply)
		if ok && reply.Err == OK {
			return
		}

		ck.primary = ""
		time.Sleep(viewservice.PingInterval)
	}
}

// tell the primary to update key's value.
// must keep trying until it succeeds.
func (ck *Clerk) Put(key string, value string) {
	ck.PutAppend(key, value, "Put")
}

// tell the primary to append to key's value.
// must keep trying until it succeeds.
func (ck *Clerk) Append(key string, value string) {
	ck.PutAppend(key, value, "Append")
}
