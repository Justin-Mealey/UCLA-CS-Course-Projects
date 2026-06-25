package shardmaster

import (
	"cs134-kv/paxos"
	"encoding/gob"
	"fmt"
	"log"
	"math/rand"
	"net"
	"net/rpc"
	"os"
	"sort"
	"sync"
	"sync/atomic"
	"syscall"
	"time"
)

type ShardMaster struct {
	mu         sync.Mutex
	l          net.Listener
	me         int
	dead       int32 // for testing
	unreliable int32 // for testing
	px         *paxos.Paxos

	configs []Config // indexed by config num
}

type Op struct {
	Type    string // "Join", "Leave", "Move", or "Query"
	GID     int64
	Servers []string
	Shard   int
}

// Helper to balance shards among groups
func (sm *ShardMaster) balanceShards(prevConfig *Config, groups map[int64][]string) [NShards]int64 {
	shardGid := [NShards]int64{}

	if len(groups) == 0 {
		// all shards get GID 0
		for i := 0; i < NShards; i++ {
			shardGid[i] = 0
		}
		return shardGid
	}

	target := NShards / len(groups) // ideal shard count per group, load balanced
	remainder := NShards % len(groups)

	// Keep shards where they are if possible
	for i := 0; i < NShards; i++ {
		oldGID := prevConfig.Shards[i]
		if _, ok := groups[oldGID]; ok {
			shardGid[i] = oldGID
		} else {
			// Group is down, mark shard for reassignment
			shardGid[i] = 0
		}
	}

	// Track unassigned shards
	unassignedShards := make([]int, 0)
	for i := 0; i < NShards; i++ {
		_, ok := groups[shardGid[i]]
		if shardGid[i] == 0 || !ok {
			unassignedShards = append(unassignedShards, i)
		}
	}

	// Calculate shard counts for active groups
	shardCount := make(map[int64]int)
	for i := 0; i < NShards; i++ {
		_, ok := groups[shardGid[i]]
		if ok {
			shardCount[shardGid[i]]++
		}
	}

	// Sort groups
	var sortedGIDs []int64
	for gid := range groups {
		sortedGIDs = append(sortedGIDs, gid)
	}
	sort.Slice(sortedGIDs, func(i, j int) bool {
		return sortedGIDs[i] < sortedGIDs[j]
	})

	// Naive assignment
	for _, shard := range unassignedShards {
		minGID := sortedGIDs[0]
		minCount := shardCount[minGID]

		for _, gid := range sortedGIDs {
			if shardCount[gid] < minCount {
				minCount = shardCount[gid]
				minGID = gid
			} else if shardCount[gid] == minCount && gid < minGID {
				minGID = gid
			}
		}

		shardGid[shard] = minGID
		shardCount[minGID]++
	}

	// Tuning, transfer shards to balance load
	for {
		changed := false
		for _, gid := range sortedGIDs {
			targetCount := target
			if gid <= int64(remainder) {
				targetCount = target + 1
			}

			if shardCount[gid] > targetCount { // Want to find shards to move out
				for i := 0; i < NShards; i++ {
					if shardGid[i] == gid {
						// Find a group that needs more shards
						for _, targetGID := range sortedGIDs {
							targetCount2 := target
							if targetGID <= int64(remainder) {
								targetCount2 = target + 1
							}

							if shardCount[targetGID] < targetCount2 {
								// Switch shard
								shardGid[i] = targetGID
								shardCount[gid]--
								shardCount[targetGID]++
								changed = true
								break
							}
						}
						if changed {
							break
						}
					}
				}
				if changed {
					break
				}
			}
		}
		if !changed {
			break
		}
	}

	return shardGid
}

func (sm *ShardMaster) executeOp(op Op) {
	for {
		seq := sm.px.Max() + 1
		sm.px.Start(seq, op)

		// Wait for paxos protocol to agree
		sm.mu.Unlock()
		to := 10 * time.Millisecond
		for {
			status, _ := sm.px.Status(seq)
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
		sm.mu.Lock()

		status, val := sm.px.Status(seq)
		if status != paxos.Decided {
			continue
		}

		decidedOp, ok := val.(Op)
		if !ok {
			continue
		}

		sm.applyUpTo(seq)

		// Only exit once operation succeeds
		if sm.operationMatches(decidedOp, op) {
			return
		}
	}
}

// Helper function to check if two operations match
func (sm *ShardMaster) operationMatches(op1, op2 Op) bool {
	if op1.Type != op2.Type {
		return false
	}
	if op1.GID != op2.GID {
		return false
	}

	switch op1.Type {
	case "Join":
		if len(op1.Servers) != len(op2.Servers) {
			return false
		}
		for i := range op1.Servers {
			if op1.Servers[i] != op2.Servers[i] {
				return false
			}
		}
		return true
	case "Leave":
		return true
	case "Move":
		return op1.Shard == op2.Shard
	case "NoOp":
		return true
	default:
		return false
	}
}

// Apply all operations from after highest_applied_instance to targetSeq
func (sm *ShardMaster) applyUpTo(targetSeq int) {

	currentSeq := len(sm.configs) - 1

	for {
		if currentSeq > targetSeq {
			break
		}

		status, val := sm.px.Status(currentSeq)

		if status == paxos.Decided {
			op := val.(Op)
			prevConfig := sm.configs[len(sm.configs)-1]

			newConfig := Config{
				Num:    prevConfig.Num + 1,
				Shards: prevConfig.Shards,
				Groups: make(map[int64][]string),
			}

			for gid, servers := range prevConfig.Groups {
				newConfig.Groups[gid] = append([]string{}, servers...)
			}

			// Apply op for currentSeq
			switch op.Type {
			case "Join":
				newServers := append([]string{}, op.Servers...)
				newConfig.Groups[op.GID] = newServers

				// Rebalance shards
				newConfig.Shards = sm.balanceShards(&prevConfig, newConfig.Groups)

			case "Leave":
				// Remove the group
				delete(newConfig.Groups, op.GID)

				// Rebalance shards
				newConfig.Shards = sm.balanceShards(&prevConfig, newConfig.Groups)

			case "Move":
				// Move specific shard
				newConfig.Shards[op.Shard] = op.GID
			}

			sm.configs = append(sm.configs, newConfig)
			sm.px.Done(currentSeq)
		} else if status == paxos.Pending {
			// we have applied all decided ops, current op isn't decided yet
			break
		} else {
			// paxos.Forgotten case, just move to next seq
		}

		currentSeq++
	}
}

func (sm *ShardMaster) Join(args *JoinArgs, reply *JoinReply) error {
	sm.mu.Lock()
	defer sm.mu.Unlock()

	op := Op{
		Type:    "Join",
		GID:     args.GID,
		Servers: args.Servers,
	}

	sm.executeOp(op)
	return nil
}

func (sm *ShardMaster) Leave(args *LeaveArgs, reply *LeaveReply) error {
	sm.mu.Lock()
	defer sm.mu.Unlock()

	op := Op{
		Type: "Leave",
		GID:  args.GID,
	}

	sm.executeOp(op)
	return nil
}

func (sm *ShardMaster) Move(args *MoveArgs, reply *MoveReply) error {
	sm.mu.Lock()
	defer sm.mu.Unlock()

	op := Op{
		Type:  "Move",
		Shard: args.Shard,
		GID:   args.GID,
	}

	sm.executeOp(op)
	return nil
}

func (sm *ShardMaster) Query(args *QueryArgs, reply *QueryReply) error {
	sm.mu.Lock()
	defer sm.mu.Unlock()

	// Force fresh state with noop
	noop := Op{Type: "NoOp"}
	seq := sm.px.Max() + 1
	sm.px.Start(seq, noop)

	sm.mu.Unlock()
	to := 10 * time.Millisecond
	for {
		status, _ := sm.px.Status(seq)
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
	sm.mu.Lock()

	sm.applyUpTo(seq)

	// Now we are fresh and can query
	num := args.Num
	if num < 0 || num >= len(sm.configs) {
		reply.Config = sm.configs[len(sm.configs)-1]
	} else {
		reply.Config = sm.configs[num]
	}

	return nil
}

// please don't change these two functions.
func (sm *ShardMaster) Kill() {
	atomic.StoreInt32(&sm.dead, 1)
	sm.l.Close()
	sm.px.Kill()
}

// call this to find out if the server is dead.
func (sm *ShardMaster) isdead() bool {
	return atomic.LoadInt32(&sm.dead) != 0
}

// please do not change these two functions.
func (sm *ShardMaster) setunreliable(what bool) {
	if what {
		atomic.StoreInt32(&sm.unreliable, 1)
	} else {
		atomic.StoreInt32(&sm.unreliable, 0)
	}
}

func (sm *ShardMaster) isunreliable() bool {
	return atomic.LoadInt32(&sm.unreliable) != 0
}

// servers[] contains the ports of the set of
// servers that will cooperate via Paxos to
// form the fault-tolerant shardmaster service.
// me is the index of the current server in servers[].
func StartServer(servers []string, me int) *ShardMaster {
	sm := new(ShardMaster)
	sm.me = me

	sm.configs = make([]Config, 1)
	sm.configs[0].Groups = map[int64][]string{}

	rpcs := rpc.NewServer()

	gob.Register(Op{})
	rpcs.Register(sm)
	sm.px = paxos.Make(servers, me, rpcs)

	os.Remove(servers[me])
	l, e := net.Listen("unix", servers[me])
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	sm.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for sm.isdead() == false {
			conn, err := sm.l.Accept()
			if err == nil && sm.isdead() == false {
				if sm.isunreliable() && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if sm.isunreliable() && (rand.Int63()%1000) < 200 {
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
			if err != nil && sm.isdead() == false {
				fmt.Printf("ShardMaster(%v) accept: %v\n", me, err.Error())
				sm.Kill()
			}
		}
	}()

	return sm
}
