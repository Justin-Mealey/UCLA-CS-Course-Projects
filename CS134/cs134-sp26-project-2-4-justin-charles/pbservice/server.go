package pbservice

import "net"
import "fmt"
import "net/rpc"
import "log"
import "time"
import "cs134-kv/viewservice"
import "sync"
import "sync/atomic"
import "os"
import "syscall"
import "math/rand"

type PBServer struct {
	mu         sync.Mutex
	l          net.Listener
	dead       int32 // for testing
	unreliable int32 // for testing
	me         string
	vs         *viewservice.Clerk
	view       viewservice.View
	db         map[string]string
	dup        map[int64]string
	lastBackup string
}

func (pb *PBServer) Get(args *GetArgs, reply *GetReply) error {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	if pb.view.Primary == "" || pb.me != pb.view.Primary {
		reply.Err = ErrWrongServer
		return nil
	}

	if val, ok := pb.dup[args.Id]; ok {
		reply.Value = val
		reply.Err = OK
		return nil
	}

	if pb.view.Backup != "" {
		fwdArgs := &ForwardGetArgs{
			Key:      args.Key,
			Id:       args.Id,
			ClientId: args.ClientId,
		}
		var fwdReply ForwardGetReply
		ok := call(pb.view.Backup, "PBServer.ForwardGet", fwdArgs, &fwdReply)
		if !ok || fwdReply.Err != OK {
			reply.Err = ErrWrongServer
			return nil
		}
	}

	val, exists := pb.db[args.Key]
	if exists {
		reply.Err = OK
		reply.Value = val
	} else {
		reply.Err = ErrNoKey
		reply.Value = ""
	}
	pb.dup[args.Id] = reply.Value

	return nil
}

func (pb *PBServer) PutAppend(args *PutAppendArgs, reply *PutAppendReply) error {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	if pb.view.Primary == "" || pb.me != pb.view.Primary {
		reply.Err = ErrWrongServer
		return nil
	}

	if _, ok := pb.dup[args.Id]; ok {
		if pb.view.Backup != "" {
			if !pb.sendState(pb.view.Backup) {
				reply.Err = ErrWrongServer
				return nil
			}
		}
		reply.Err = OK
		return nil
	}

	if args.Op == "Put" {
		pb.db[args.Key] = args.Value
	} else {
		pb.db[args.Key] = pb.db[args.Key] + args.Value
	}
	pb.dup[args.Id] = pb.db[args.Key]

	if pb.view.Backup != "" {
		fwdArgs := &ForwardPutAppendArgs{
			Key:      args.Key,
			Value:    args.Value,
			Op:       args.Op,
			Id:       args.Id,
			ClientId: args.ClientId,
		}
		var fwdReply ForwardPutAppendReply
		ok := call(pb.view.Backup, "PBServer.ForwardPutAppend", fwdArgs, &fwdReply)
		if !ok || fwdReply.Err != OK {
			if !pb.sendState(pb.view.Backup) {
				reply.Err = ErrWrongServer
				return nil
			}
		}
	}

	reply.Err = OK
	return nil
}

func (pb *PBServer) sendState(backup string) bool {
	dbCopy := make(map[string]string)
	for k, v := range pb.db {
		dbCopy[k] = v
	}
	dupCopy := make(map[int64]string)
	for k, v := range pb.dup {
		dupCopy[k] = v
	}
	args := &TransferArgs{DB: dbCopy, Dup: dupCopy}
	var reply TransferReply
	ok := call(backup, "PBServer.Transfer", args, &reply)
	return ok && reply.Err == OK
}

func (pb *PBServer) ForwardGet(args *ForwardGetArgs, reply *ForwardGetReply) error {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	if pb.me != pb.view.Backup {
		reply.Err = ErrWrongServer
		return nil
	}

	if _, ok := pb.dup[args.Id]; ok {
		reply.Err = OK
		return nil
	}

	pb.dup[args.Id] = pb.db[args.Key]
	reply.Err = OK
	return nil
}

func (pb *PBServer) ForwardPutAppend(args *ForwardPutAppendArgs, reply *ForwardPutAppendReply) error {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	if pb.me != pb.view.Backup {
		reply.Err = ErrWrongServer
		return nil
	}

	if _, ok := pb.dup[args.Id]; ok {
		reply.Err = OK
		return nil
	}

	if args.Op == "Put" {
		pb.db[args.Key] = args.Value
	} else if args.Op == "Append" {
		pb.db[args.Key] = pb.db[args.Key] + args.Value
	}
	pb.dup[args.Id] = pb.db[args.Key]
	reply.Err = OK

	return nil
}

func (pb *PBServer) Transfer(args *TransferArgs, reply *TransferReply) error {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	pb.db = args.DB
	pb.dup = args.Dup
	reply.Err = OK

	return nil
}

func (pb *PBServer) tick() {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	v, err := pb.vs.Ping(pb.view.Viewnum)
	if err != nil {
		return
	}

	pb.view = v

	if v.Primary != pb.me {
		pb.lastBackup = ""
		return
	}

	if v.Backup != "" && v.Backup != pb.lastBackup {
		if pb.sendState(v.Backup) {
			pb.lastBackup = v.Backup
		}
	}
}

// tell the server to shut itself down.
// please do not change these two functions.
func (pb *PBServer) kill() {
	atomic.StoreInt32(&pb.dead, 1)
	pb.l.Close()
}

// call this to find out if the server is dead.
func (pb *PBServer) isdead() bool {
	return atomic.LoadInt32(&pb.dead) != 0
}

// please do not change these two functions.
func (pb *PBServer) setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&pb.unreliable, 1)
	} else {
		atomic.StoreInt32(&pb.unreliable, 0)
	}
}

func (pb *PBServer) isunreliable() bool {
	return atomic.LoadInt32(&pb.unreliable) != 0
}

func StartServer(vshost string, me string) *PBServer {
	pb := new(PBServer)
	pb.me = me
	pb.vs = viewservice.MakeClerk(me, vshost)
	pb.db = make(map[string]string)
	pb.dup = make(map[int64]string)

	rpcs := rpc.NewServer()
	rpcs.Register(pb)

	os.Remove(pb.me)
	l, e := net.Listen("unix", pb.me)
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	pb.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for pb.isdead() == false {
			conn, err := pb.l.Accept()
			if err == nil && pb.isdead() == false {
				if pb.isunreliable() && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if pb.isunreliable() && (rand.Int63()%1000) < 200 {
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
			if err != nil && pb.isdead() == false {
				fmt.Printf("PBServer(%v) accept: %v\n", me, err.Error())
				pb.kill()
			}
		}
	}()

	go func() {
		for pb.isdead() == false {
			pb.tick()
			time.Sleep(viewservice.PingInterval)
		}
	}()

	return pb
}
