package viewservice

// Modified by VSCode Extension
import (
	"fmt"
	"log"
	"net"
	"net/rpc"
	"os"
	"sync"
	"sync/atomic"
	"time"
)

type ViewServer struct {
	mu       sync.Mutex
	l        net.Listener
	dead     int32 // for testing
	rpccount int32 // for testing
	me       string

	// Your declarations here.
	currentView   View
	pingTime      map[string]time.Time
	primaryAcked  bool            // has primary acked current view?
	serverViewnum map[string]uint // server -> last viewnum seen by server
	serverCrashed map[string]bool // server -> crashed?
}

// server Ping RPC handler.
func (vs *ViewServer) Ping(args *PingArgs, reply *PingReply) error {
	// Nonstarter code
	vs.mu.Lock()
	defer vs.mu.Unlock()

	vs.pingTime[args.Me] = time.Now()

	lastViewnum, exists := vs.serverViewnum[args.Me]

	// Crash detection
	if args.Viewnum == 0 && exists && lastViewnum != 0 {
		vs.serverCrashed[args.Me] = true
	}

	vs.serverViewnum[args.Me] = args.Viewnum

	// Check if primary is acknowledging the current view
	if args.Me == vs.currentView.Primary && args.Viewnum == vs.currentView.Viewnum {
		vs.primaryAcked = true
	}

	// Simply reply to server with current view
	reply.View = vs.currentView
	return nil
}

// server Get() RPC handler.
func (vs *ViewServer) Get(args *GetArgs, reply *GetReply) error {
	// Nonstarter code
	vs.mu.Lock()
	defer vs.mu.Unlock()
	// TODO: actually Get() the data
	reply.View = vs.currentView
	return nil
}

// tick() is called once per PingInterval; it should notice
// if servers have died or recovered, and change the view
// accordingly.
func (vs *ViewServer) tick() {
	// Nonstarter code
	vs.mu.Lock()
	defer vs.mu.Unlock()

	// View 0, select any available server to be primary
	if vs.currentView.Viewnum == 0 && vs.currentView.Primary == "" {
		for server := range vs.pingTime {
			vs.currentView.Primary = server
			vs.currentView.Viewnum = 1
			vs.primaryAcked = false
			return
		}
	}

	now := time.Now()
	// Longest amount of time with no ping before we think a server is dead
	deadTime := time.Duration(DeadPings) * PingInterval

	// Primary crashed/dead detection
	primaryDead := false
	if vs.currentView.Primary != "" {
		lastPing, exists := vs.pingTime[vs.currentView.Primary]
		if !exists || now.Sub(lastPing) > deadTime {
			primaryDead = true
		}

		if vs.serverCrashed[vs.currentView.Primary] {
			primaryDead = true
			vs.serverCrashed[vs.currentView.Primary] = false // clear the flag
		}
	}

	// Backup dead detection
	backupDead := false
	if vs.currentView.Backup != "" {
		lastPing, exists := vs.pingTime[vs.currentView.Backup]
		if !exists || now.Sub(lastPing) > deadTime {
			backupDead = true
		}
	}

	// If primary is dead and we've acked current view, promote
	// Following spec: "But the view service must not change views
	// (i.e., return a different view to callers) until the primary
	// from the current view acknowledges that it is operating in the current view"
	if primaryDead && vs.primaryAcked {
		vs.promoteView()
		return
	}

	// Replace dead backup (changing the view, so primary must have acked currentView)
	if backupDead && vs.primaryAcked && vs.currentView.Backup != "" {
		for server := range vs.pingTime {
			if server != vs.currentView.Primary && server != vs.currentView.Backup {
				// Ensure server is alive and pinging before making it Backup
				lastPing, exists := vs.pingTime[server]
				if exists && now.Sub(lastPing) <= deadTime {
					vs.currentView.Backup = server
					vs.currentView.Viewnum++
					vs.primaryAcked = false
					return
				}
			}
		}
		// No good Backup options found
		vs.currentView.Backup = ""
		vs.currentView.Viewnum++
		vs.primaryAcked = false
		return
	}

	// Fill empty Backup role
	if vs.currentView.Backup == "" && vs.primaryAcked {
		for server := range vs.pingTime {
			if server != vs.currentView.Primary {
				// Ensure server is alive and pinging before making it Backup
				lastPing, exists := vs.pingTime[server]
				if exists && now.Sub(lastPing) <= deadTime {
					vs.currentView.Backup = server
					vs.currentView.Viewnum++
					vs.primaryAcked = false
					return
				}
			}
		}
	}
}

// Helper function to promote Backup -> Primary
func (vs *ViewServer) promoteView() {
	if vs.currentView.Backup != "" {
		vs.currentView.Primary = vs.currentView.Backup
		vs.currentView.Backup = ""
		vs.currentView.Viewnum++
		vs.primaryAcked = false
		// We set backup NOT here, but on later tick
		// as new Primary must ack currentView before we change the view again
	}
}

// tell the server to shut itself down.
// for testing.
// please don't change these two functions.
func (vs *ViewServer) Kill() {
	atomic.StoreInt32(&vs.dead, 1)
	vs.l.Close()
}

// has this server been asked to shut down?
func (vs *ViewServer) isdead() bool {
	return atomic.LoadInt32(&vs.dead) != 0
}

// please don't change this function.
func (vs *ViewServer) GetRPCCount() int32 {
	return atomic.LoadInt32(&vs.rpccount)
}

func StartServer(me string) *ViewServer {
	vs := new(ViewServer)
	vs.me = me
	// Your vs.* initializations here.
	vs.currentView = View{Viewnum: 0, Primary: "", Backup: ""}
	vs.pingTime = make(map[string]time.Time)
	vs.serverViewnum = make(map[string]uint)
	vs.serverCrashed = make(map[string]bool)
	vs.primaryAcked = false

	// tell net/rpc about our RPC server and handlers.
	rpcs := rpc.NewServer()
	rpcs.Register(vs)

	// prepare to receive connections from clients.
	// change "unix" to "tcp" to use over a network.
	os.Remove(vs.me) // only needed for "unix"
	l, e := net.Listen("unix", vs.me)
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	vs.l = l

	// please don't change any of the following code,
	// or do anything to subvert it.

	// create a thread to accept RPC connections from clients.
	go func() {
		for vs.isdead() == false {
			conn, err := vs.l.Accept()
			if err == nil && vs.isdead() == false {
				atomic.AddInt32(&vs.rpccount, 1)
				go rpcs.ServeConn(conn)
			} else if err == nil {
				conn.Close()
			}
			if err != nil && vs.isdead() == false {
				fmt.Printf("ViewServer(%v) accept: %v\n", me, err.Error())
				vs.Kill()
			}
		}
	}()

	// create a thread to call tick() periodically.
	go func() {
		for vs.isdead() == false {
			vs.tick()
			time.Sleep(PingInterval)
		}
	}()

	return vs
}
