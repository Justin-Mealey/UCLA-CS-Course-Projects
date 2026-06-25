package paxos

//
// Paxos library, to be included in an application.
// Multiple applications will run, each including
// a Paxos peer.
//
// Manages a sequence of agreed-on values.
// The set of peers is fixed.
// Copes with network failures (partition, msg loss, &c).
// Does not store anything persistently, so cannot handle crash+restart.
//
// The application interface:
//
// px = paxos.Make(peers []string, me string)
// px.Start(seq int, v interface{}) -- start agreement on new instance
// px.Status(seq int) (Fate, v interface{}) -- get info about an instance
// px.Done(seq int) -- ok to forget all instances <= seq
// px.Max() int -- highest instance seq known, or -1
// px.Min() int -- instances before this seq have been forgotten
//

import (
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

// px.Status() return values, indicating
// whether an agreement has been decided,
// or Paxos has not yet reached agreement,
// or it was agreed but forgotten (i.e. < Min()).
type Fate int

const (
	Decided   Fate = iota + 1
	Pending        // not yet decided.
	Forgotten      // decided but forgotten.
)

type PxInstance struct {
	HighestPrep  int
	AcceptNum    int
	AcceptVal    interface{}
	IsDecided    bool
	DecidedValue interface{}
}

// rpc structs
type PrepareArgs struct {
	Seq     int
	PropNum int
	Sender  int
	DoneVal int
}
type PrepareReply struct {
	Accepted bool
	AccNum   int
	AccVal   interface{}
}

type AcceptArgs struct {
	Seq     int
	PropNum int
	Val     interface{}
	Sender  int
	DoneVal int
}
type AcceptReply struct {
	Accepted bool
}

type LearnArgs struct {
	Seq     int
	Val     interface{}
	Sender  int
	DoneVal int
}
type LearnReply struct {
	OK bool
}

type Paxos struct {
	mu         sync.Mutex
	l          net.Listener
	dead       int32 // for testing
	unreliable int32 // for testing
	rpcCount   int32 // for testing
	peers      []string
	me         int // index into peers[]

	log         map[int]*PxInstance
	peerDone    []int
	highestSeen int
}

// call() sends an RPC to the rpcname handler on server srv
// with arguments args, waits for the reply, and leaves the
// reply in reply. the reply argument should be a pointer
// to a reply structure.
//
// the return value is true if the server responded, and false
// if call() was not able to contact the server. in particular,
// the replys contents are only valid if call() returned true.
//
// you should assume that call() will time out and return an
// error after a while if it does not get a reply from the server.
//
// please use call() to send all RPCs, in client.go and server.go.
// please do not change this function except to comment out the fmt.Println or fmt.Printf line.
func call(srv string, name string, args interface{}, reply interface{}) bool {
	c, err := rpc.Dial("unix", srv)
	if err != nil {
		err1 := err.(*net.OpError)
		if err1.Err != syscall.ENOENT && err1.Err != syscall.ECONNREFUSED {
			// fmt.Printf("paxos Dial() failed: %v\n", err1)
		}
		return false
	}
	defer c.Close()

	err = c.Call(name, args, reply)
	if err == nil {
		return true
	}

	// fmt.Println("paxos/paxos.go/call():", err)
	return false
}

func (px *Paxos) getOrCreate(seq int) *PxInstance {
	if px.log[seq] == nil {
		px.log[seq] = &PxInstance{}
	}
	return px.log[seq]
}

func (px *Paxos) recordPeerDone(who int, val int) {
	if val > px.peerDone[who] {
		px.peerDone[who] = val
	}
}

func (px *Paxos) globalMin() int {
	lowest := px.peerDone[0]
	for j := 1; j < len(px.peerDone); j++ {
		if px.peerDone[j] < lowest {
			lowest = px.peerDone[j]
		}
	}
	return lowest + 1
}

// acceptor: handle prepare
func (px *Paxos) HandlePrepare(args *PrepareArgs, reply *PrepareReply) error {
	px.mu.Lock()
	defer px.mu.Unlock()

	px.recordPeerDone(args.Sender, args.DoneVal)
	if args.Seq > px.highestSeen {
		px.highestSeen = args.Seq
	}

	slot := px.getOrCreate(args.Seq)
	if args.PropNum > slot.HighestPrep {
		slot.HighestPrep = args.PropNum
		reply.Accepted = true
		reply.AccNum = slot.AcceptNum
		reply.AccVal = slot.AcceptVal
	} else {
		reply.Accepted = false
	}
	return nil
}

// acceptor: handle accept
func (px *Paxos) HandleAccept(args *AcceptArgs, reply *AcceptReply) error {
	px.mu.Lock()
	defer px.mu.Unlock()

	px.recordPeerDone(args.Sender, args.DoneVal)
	if args.Seq > px.highestSeen {
		px.highestSeen = args.Seq
	}

	slot := px.getOrCreate(args.Seq)
	if args.PropNum >= slot.HighestPrep {
		slot.HighestPrep = args.PropNum
		slot.AcceptNum = args.PropNum
		slot.AcceptVal = args.Val
		reply.Accepted = true
	} else {
		reply.Accepted = false
	}
	return nil
}

// learner: handle decided
func (px *Paxos) HandleDecide(args *LearnArgs, reply *LearnReply) error {
	px.mu.Lock()
	defer px.mu.Unlock()

	px.recordPeerDone(args.Sender, args.DoneVal)
	if args.Seq > px.highestSeen {
		px.highestSeen = args.Seq
	}

	slot := px.getOrCreate(args.Seq)
	slot.IsDecided = true
	slot.DecidedValue = args.Val
	reply.OK = true
	return nil
}

func (px *Paxos) driveInstance(seq int, val interface{}) {
	r := 0
	for px.isdead() == false {
		// already done?
		px.mu.Lock()
		s := px.getOrCreate(seq)
		if s.IsDecided {
			px.mu.Unlock()
			return
		}
		myDone := px.peerDone[px.me]
		px.mu.Unlock()

		px.cleanupLog()

		propN := r*len(px.peers) + px.me
		majority := len(px.peers)/2 + 1

		// --- phase 1: prepare ---
		yesCount := 0
		bestN := -1
		var bestV interface{}

		for idx := range px.peers {
			pArgs := &PrepareArgs{Seq: seq, PropNum: propN, Sender: px.me, DoneVal: myDone}
			pReply := &PrepareReply{}
			got := false
			if idx == px.me {
				px.HandlePrepare(pArgs, pReply)
				got = true
			} else {
				got = call(px.peers[idx], "Paxos.HandlePrepare", pArgs, pReply)
			}
			if got && pReply.Accepted {
				yesCount++
				if pReply.AccNum > bestN {
					bestN = pReply.AccNum
					bestV = pReply.AccVal
				}
			}
		}

		if yesCount < majority {
			r++
			time.Sleep(time.Duration(rand.Intn(25)+5) * time.Millisecond)
			continue
		}

		// use previously accepted value if there was one
		useVal := val
		if bestN > 0 {
			useVal = bestV
		}

		// --- phase 2: accept ---
		accCount := 0
		for idx := range px.peers {
			aArgs := &AcceptArgs{Seq: seq, PropNum: propN, Val: useVal, Sender: px.me, DoneVal: myDone}
			aReply := &AcceptReply{}
			got := false
			if idx == px.me {
				px.HandleAccept(aArgs, aReply)
				got = true
			} else {
				got = call(px.peers[idx], "Paxos.HandleAccept", aArgs, aReply)
			}
			if got && aReply.Accepted {
				accCount++
			}
		}

		if accCount < majority {
			r++
			time.Sleep(time.Duration(rand.Intn(25)+5) * time.Millisecond)
			continue
		}

		// --- phase 3: tell everyone it's decided ---
		for idx := range px.peers {
			dArgs := &LearnArgs{Seq: seq, Val: useVal, Sender: px.me, DoneVal: myDone}
			dReply := &LearnReply{}
			if idx == px.me {
				px.HandleDecide(dArgs, dReply)
			} else {
				call(px.peers[idx], "Paxos.HandleDecide", dArgs, dReply)
			}
		}
		return
	}
}

// the application wants paxos to start agreement on
// instance seq, with proposed value v.
// Start() returns right away; the application will
// call Status() to find out if/when agreement
// is reached.
func (px *Paxos) Start(seq int, v interface{}) {
	px.mu.Lock()
	if seq < px.globalMin() {
		px.mu.Unlock()
		return
	}
	if seq > px.highestSeen {
		px.highestSeen = seq
	}
	px.mu.Unlock()

	px.cleanupLog()

	go px.driveInstance(seq, v)
}

// the application on this machine is done with
// all instances <= seq.
//
// see the comments for Min() for more explanation.
func (px *Paxos) Done(seq int) {
	px.mu.Lock()

	if seq > px.peerDone[px.me] {
		px.peerDone[px.me] = seq
	}

	px.mu.Unlock()
	px.cleanupLog()
}

// the application wants to know the
// highest instance sequence known to
// this peer.
func (px *Paxos) Max() int {
	px.mu.Lock()
	defer px.mu.Unlock()
	return px.highestSeen
}

// Min() should return one more than the minimum among z_i,
// where z_i is the highest number ever passed
// to Done() on peer i. A peers z_i is -1 if it has
// never called Done().
//
// Paxos is required to have forgotten all information
// about any instances it knows that are < Min().
// The point is to free up memory in long-running
// Paxos-based servers.
//
// Paxos peers need to exchange their highest Done()
// arguments in order to implement Min(). These
// exchanges can be piggybacked on ordinary Paxos
// agreement protocol messages, so it is OK if one
// peers Min does not reflect another Peers Done()
// until after the next instance is agreed to.
//
// The fact that Min() is defined as a minimum over
// *all* Paxos peers means that Min() cannot increase until
// all peers have been heard from. So if a peer is dead
// or unreachable, other peers Min()s will not increase
// even if all reachable peers call Done. The reason for
// this is that when the unreachable peer comes back to
// life, it will need to catch up on instances that it
// missed -- the other peers therefor cannot forget these
// instances.
func (px *Paxos) Min() int {
	px.mu.Lock()
	defer px.mu.Unlock()

	gmin := px.globalMin()

	// free old instances
	for s := range px.log {
		if s < gmin {
			delete(px.log, s)
		}
	}
	return gmin
}

// the application wants to know whether this
// peer thinks an instance has been decided,
// and if so what the agreed value is. Status()
// should just inspect the local peer state;
// it should not contact other Paxos peers.
func (px *Paxos) Status(seq int) (Fate, interface{}) {
	px.mu.Lock()

	if seq < px.globalMin() {
		px.mu.Unlock()
		px.cleanupLog()
		return Forgotten, nil
	}
	entry := px.log[seq]
	if entry != nil && entry.IsDecided {
		decidedVal := entry.DecidedValue
		px.mu.Unlock()
		px.cleanupLog()
		return Decided, decidedVal
	}
	px.mu.Unlock()
	px.cleanupLog()
	return Pending, nil
}

// tell the peer to shut itself down.
// for testing.
// please do not change these two functions.
func (px *Paxos) Kill() {
	atomic.StoreInt32(&px.dead, 1)
	if px.l != nil {
		px.l.Close()
	}
}

// has this peer been asked to shut down?
func (px *Paxos) isdead() bool {
	return atomic.LoadInt32(&px.dead) != 0
}

// please do not change these two functions.
func (px *Paxos) setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&px.unreliable, 1)
	} else {
		atomic.StoreInt32(&px.unreliable, 0)
	}
}

func (px *Paxos) isunreliable() bool {
	return atomic.LoadInt32(&px.unreliable) != 0
}

// the application wants to create a paxos peer.
// the ports of all the paxos peers (including this one)
// are in peers[]. this servers port is peers[me].
func Make(peers []string, me int, rpcs *rpc.Server) *Paxos {
	px := &Paxos{}
	px.peers = peers
	px.me = me

	px.log = make(map[int]*PxInstance)
	px.peerDone = make([]int, len(peers))
	for k := range px.peerDone {
		px.peerDone[k] = -1
	}
	px.highestSeen = -1

	if rpcs != nil {
		// caller will create socket &c
		rpcs.Register(px)
	} else {
		rpcs = rpc.NewServer()
		rpcs.Register(px)

		// prepare to receive connections from clients.
		// change "unix" to "tcp" to use over a network.
		os.Remove(peers[me]) // only needed for "unix"
		l, e := net.Listen("unix", peers[me])
		if e != nil {
			log.Fatal("listen error: ", e)
		}
		px.l = l

		// please do not change any of the following code,
		// or do anything to subvert it.

		// create a thread to accept RPC connections
		go func() {
			for px.isdead() == false {
				conn, err := px.l.Accept()
				if err == nil && px.isdead() == false {
					if px.isunreliable() && (rand.Int63()%1000) < 100 {
						// discard the request.
						conn.Close()
					} else if px.isunreliable() && (rand.Int63()%1000) < 200 {
						// process the request but force discard of reply.
						c1 := conn.(*net.UnixConn)
						f, _ := c1.File()
						err := syscall.Shutdown(int(f.Fd()), syscall.SHUT_WR)
						if err != nil {
							fmt.Printf("shutdown: %v\n", err)
						}
						atomic.AddInt32(&px.rpcCount, 1)
						go rpcs.ServeConn(conn)
					} else {
						atomic.AddInt32(&px.rpcCount, 1)
						go rpcs.ServeConn(conn)
					}
				} else if err == nil {
					conn.Close()
				}
				if err != nil && px.isdead() == false {
					fmt.Printf("Paxos(%v) accept: %v\n", me, err.Error())
				}
			}
		}()
	}

	return px
}

func (px *Paxos) cleanupLog() {
	px.mu.Lock()
	defer px.mu.Unlock()

	gmin := px.globalMin()
	for s := range px.log {
		if s < gmin {
			delete(px.log, s)
		}
	}
}
