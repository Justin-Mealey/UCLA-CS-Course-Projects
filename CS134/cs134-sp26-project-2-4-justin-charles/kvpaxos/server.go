package kvpaxos

import (
	"cs134-kv/paxos"
	"encoding/gob"
	"fmt"
	"log"
	"math/rand"
	"net"
	"net/rpc"
	"os"
	"sync"
	"sync/atomic"
	"syscall"
	"time"
)

const Debug = 0

func DPrintf(format string, a ...interface{}) (n int, err error) {
	if Debug > 0 {
		log.Printf(format, a...)
	}
	return
}

type Op struct {
	// TODO: Your definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	Type      string // "Get", "Put", or "Append"
	Key       string
	Value     string
	ClientId  int64
	RequestId int64
}

type KVPaxos struct {
	mu         sync.Mutex
	l          net.Listener
	me         int
	dead       int32 // for testing
	unreliable int32 // for testing
	px         *paxos.Paxos

	// TODO: Your definitions here.
	db                       map[string]string // key -> value database
	highest_applied_instance int
	lastReq                  map[int64]int64 // clientId -> last requestId we've processed
}

// Apply all operations from after highest_applied_instance (up to targetSeq)
func (kv *KVPaxos) applyUpTo(targetSeq int) {
	for {
		seq := kv.highest_applied_instance + 1
		if seq > targetSeq {
			break
		}

		status, val := kv.px.Status(seq)

		if status == paxos.Decided {
			op := val.(Op)
			// Apply the operation if it's new
			lastReqId, exists := kv.lastReq[op.ClientId]
			if !exists || lastReqId < op.RequestId {
				switch op.Type {
				case "Put":
					kv.db[op.Key] = op.Value
				case "Append":
					kv.db[op.Key] += op.Value
				case "Get":
					// No DB changes needed
				}
				kv.lastReq[op.ClientId] = op.RequestId
			}
			kv.highest_applied_instance = seq
			kv.px.Done(kv.highest_applied_instance)
		} else if status == paxos.Pending {
			// Fill the gap by calling Start with a no-op
			noop := Op{Type: "Get", Key: "", Value: "", ClientId: 0, RequestId: 0}
			kv.px.Start(seq, noop)

			// Release lock and wait for Paxos protocol to stop Pending
			kv.mu.Unlock()
			to := 10 * time.Millisecond
			for {
				status, _ := kv.px.Status(seq)
				if status == paxos.Decided {
					break
				}
				if status == paxos.Forgotten {
					break
				}
				time.Sleep(to)
				if to < 10*time.Second {
					to *= 2
				}
			}
			kv.mu.Lock()
			// Next iter will process now that this Pending op is Decided
		} else {
			fmt.Printf("ERROR: Forgotten seq=%d highest_applied_instance=%d\n", seq, kv.highest_applied_instance)
		}
	}
}

func (kv *KVPaxos) Get(args *GetArgs, reply *GetReply) error {
	kv.mu.Lock()
	defer kv.mu.Unlock()

	op := Op{
		Type:      "Get",
		Key:       args.Key,
		Value:     "",
		ClientId:  args.ClientId,
		RequestId: args.RequestId,
	}

	// Keep trying with new higher seqs until we get request decided
	for {
		seq := kv.px.Max() + 1
		kv.px.Start(seq, op)

		// Wait for paxos protocol to agree
		kv.mu.Unlock()
		to := 10 * time.Millisecond
		for {
			status, _ := kv.px.Status(seq)
			if status == paxos.Decided {
				break
			}
			if status == paxos.Forgotten {
				break
			}
			time.Sleep(to)
			if to < 10*time.Second {
				to *= 2
			}
		}
		kv.mu.Lock()

		status, val := kv.px.Status(seq)
		if status != paxos.Decided {
			continue
		}
		decidedOp, ok := val.(Op)
		if !ok {
			fmt.Printf("bad cast seq=%d val=%v\n", seq, val)
			continue
		}

		// Check if this was our operation
		if decidedOp.ClientId == args.ClientId && decidedOp.RequestId == args.RequestId {
			// Our Get was decided, apply everything up to this sequence
			kv.applyUpTo(seq)
			v, exists := kv.db[args.Key]
			kv.px.Done(kv.highest_applied_instance)
			if exists {
				reply.Value = v
				reply.Err = OK
			} else {
				reply.Value = ""
				reply.Err = ErrNoKey
			}
			return nil
		}

		// Not our operation, update DB with new info and try again
		kv.applyUpTo(seq)
		kv.px.Done(kv.highest_applied_instance)
	}
}

func (kv *KVPaxos) PutAppend(args *PutAppendArgs, reply *PutAppendReply) error {
	kv.mu.Lock()
	defer kv.mu.Unlock()

	// See if we've already served this request (duplicate protection)
	if lastReqId, exists := kv.lastReq[args.ClientId]; exists && lastReqId >= args.RequestId {
		reply.Err = OK
		return nil
	}

	op := Op{
		Type:      args.Op,
		Key:       args.Key,
		Value:     args.Value,
		ClientId:  args.ClientId,
		RequestId: args.RequestId,
	}

	// Keep trying with new higher seqs until we get request decided
	for {
		seq := kv.px.Max() + 1
		kv.px.Start(seq, op)

		// Wait for paxos protocol to agree
		kv.mu.Unlock()
		to := 10 * time.Millisecond
		for {
			status, _ := kv.px.Status(seq)
			if status == paxos.Decided {
				break
			}
			if status == paxos.Forgotten {
				break
			}
			time.Sleep(to)
			if to < 10*time.Second {
				to *= 2
			}
		}
		kv.mu.Lock()

		status, val := kv.px.Status(seq)
		if status != paxos.Decided {
			continue
		}
		decidedOp, ok := val.(Op)
		if !ok {
			fmt.Printf("bad cast seq=%d val=%v\n", seq, val)
			continue
		}

		// Check if Paxos protocol just worked on our operation
		if decidedOp.ClientId == args.ClientId && decidedOp.RequestId == args.RequestId &&
			decidedOp.Type == args.Op && decidedOp.Key == args.Key && decidedOp.Value == args.Value {
			kv.applyUpTo(seq)
			kv.px.Done(kv.highest_applied_instance)
			reply.Err = OK
			return nil
		}

		// Not our operation, update DB with new info and try again
		kv.applyUpTo(seq)
		kv.px.Done(kv.highest_applied_instance)
	}
}

// tell the server to shut itself down.
// please do not change these two functions.
func (kv *KVPaxos) kill() {
	DPrintf("Kill(%d): die\n", kv.me)
	atomic.StoreInt32(&kv.dead, 1)
	kv.l.Close()
	kv.px.Kill()
}

// call this to find out if the server is dead.
func (kv *KVPaxos) isdead() bool {
	return atomic.LoadInt32(&kv.dead) != 0
}

// please do not change these two functions.
func (kv *KVPaxos) setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&kv.unreliable, 1)
	} else {
		atomic.StoreInt32(&kv.unreliable, 0)
	}
}

func (kv *KVPaxos) isunreliable() bool {
	return atomic.LoadInt32(&kv.unreliable) != 0
}

// servers[] contains the ports of the set of
// servers that will cooperate via Paxos to
// form the fault-tolerant key/value service.
// me is the index of the current server in servers[].
func StartServer(servers []string, me int) *KVPaxos {
	// call gob.Register on structures you want
	// Go's RPC library to marshall/unmarshall.
	gob.Register(Op{})

	kv := new(KVPaxos)
	kv.me = me

	// TODO: Your initialization code here.
	kv.db = make(map[string]string)
	kv.lastReq = make(map[int64]int64)
	kv.highest_applied_instance = -1

	rpcs := rpc.NewServer()
	rpcs.Register(kv)

	kv.px = paxos.Make(servers, me, rpcs)

	os.Remove(servers[me])
	l, e := net.Listen("unix", servers[me])
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	kv.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for kv.isdead() == false {
			conn, err := kv.l.Accept()
			if err == nil && kv.isdead() == false {
				if kv.isunreliable() && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if kv.isunreliable() && (rand.Int63()%1000) < 200 {
					// process the request but force discard of reply.
					c1 := conn.(*net.UnixConn)
					f, _ := c1.File()
					err := syscall.Shutdown(int(f.Fd()), syscall.SHUT_WR)
					if err != nil {
						fmt.Printf("shutdown: %v\n", err)
					}
					go rpcs.ServeConn(conn)
				} else {
					go rpcs.ServeConn(conn)
				}
			} else if err == nil {
				conn.Close()
			}
			if err != nil && kv.isdead() == false {
				fmt.Printf("KVPaxos(%v) accept: %v\n", me, err.Error())
				kv.kill()
			}
		}
	}()

	return kv
}
