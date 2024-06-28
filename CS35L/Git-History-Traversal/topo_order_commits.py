#!/usr/local/cs/bin/python3

'''
Commentary: Using strace allowed me to see if I had followed the instructions by looking at everything that ran 
when my program was invoked. When using this command, nothing from Git or any subprocesses were called, which I
confirmed by looking what was called in execve
'''

import os
import sys
import zlib

class CommitNode:
    def __init__(self, commit_hash, branches):
        self.commit_hash = commit_hash
        self.parents = set()
        self.children = set()
        self.branches = branches

# FINDERS
def find_git_directory(cd):
    git_directory = os.path.join(cd,'.git')

    if os.path.isdir(git_directory):
        return git_directory
    elif cd == os.path.abspath('/'):
        sys.stderr.write('Not inside a Git repository\n')
        exit(1)
    else:
        return find_git_directory(os.path.dirname(cd))
    
def find_local_branches(gd):
    paths = []

    branches_dir = os.path.join(gd,'refs/heads')

    if not os.path.isdir(branches_dir):
        sys.stderr.write('Invalid directory for branches\n')
        exit(1)

    for branchpath, dirs, branches in os.walk(branches_dir):
        for branch in branches:
            path_to_branch = os.path.join(branchpath, branch)
            paths.append(path_to_branch)

    return sorted(paths)
# FINDERS

# GETTERS
def get_branch_name(branches_dir, branch_path):
    common_path = os.path.commonpath([branches_dir, branch_path])
    name = os.path.relpath(branch_path, common_path)
    return name

def get_path_of_hash(gd, hash):
    path = os.path.join(gd, 'objects', hash[:2], hash[2:]) # abcd/.git/objects/a0/lastcharsofhash
    return path.strip()

def get_parents(gd, hash):
    path = get_path_of_hash(gd, hash)
    f = open(path, 'rb')
    binarydata = f.read()
    f.close()

    data = zlib.decompress(binarydata).decode('latin-1')
    data = data.split('\n')
    parents = []
    for line in data:
        line = line.split(' ')
        if line[0] == 'parent':
            parents.append(line[1]) # data looks like "parent parenthash"

    return sorted(parents) # list of parents hashes

def get_children(commit_graph, node, visited_nodes): # given full graph and node we're visiting, return node's children
    children = list(commit_graph[node].children) # list of children nodes, not just hashes but actual CommitNode objects
    unseen_children = []
    for child in children:
        if child.commit_hash not in visited_nodes:
            unseen_children.append(child)
    return unseen_children # list of children we haven't seen before
# GETTERS

def build_hash_branch_dict(gd):
    bd = os.path.join(gd,'refs/heads')
    branch_paths = find_local_branches(gd)
    
    result = {}
    
    for branch_path in branch_paths: # each branch path conatins a data file
        branch_file = open(branch_path, 'r')
        data = branch_file.read()
        hash = data[:-1] # head of the branch, last thing in file
        if hash not in result.keys(): # need to make new entry, 1st branch we've seen for this hash
            result[hash] = [get_branch_name(bd, branch_path)] # 1 item list
        else: # entry already exists, append new branch to existing branches
            result[hash].append(get_branch_name(bd, branch_path))
        branch_file.close()

    return result


def build_graphs(gd, hash_branch_dict):
    commit_graph = {}
    root_commits = set()
    for hash in hash_branch_dict:
        if hash in commit_graph.keys(): # node for hash already made
            commit_graph[hash].branches = hash_branch_dict[hash]
        else:
            branches = list(hash_branch_dict[hash])
            commit_graph[hash] = CommitNode(hash, branches)
            stack = [commit_graph[hash]]
            while len(stack) != 0:
                cur_node = stack.pop()
                parents = get_parents(gd, cur_node.commit_hash)
                if len(parents) == 0: # no parents = root commit
                    root_commits.add(cur_node.commit_hash)
                for parent in parents:
                    if parent not in commit_graph.keys(): # need to make node for the parent
                        commit_graph[parent] = CommitNode(parent, []) #update branches later if needed (when popped from stack)
                    cur_node.parents.add(commit_graph[parent]) # node get parents
                    commit_graph[parent].children.add(cur_node) # parent gets node as child
                    stack.append(commit_graph[parent]) # add parent to stack

    return list(root_commits), commit_graph


def topo_sort(root_commits, commit_graph):
    result = [] # ordered topological list
    visited_nodes = set()
    
    stack = root_commits.copy() # don't want to change the actual root commits set
    while len(stack) != 0:
        cur_node = stack[-1] # look at last, see if we can pop later
        visited_nodes.add(cur_node)
        children = get_children(commit_graph, cur_node, visited_nodes)
        if len(children) == 0: # if cur_node has no unseen children, we can remove it from stack and add it to our result
            stack.pop() 
            result.append(cur_node) 
        else: # if cur_node does have unseen children, look deeper (dfs)
            stack.append(children[0].commit_hash)
    return result


def print_topo(topo_ordering, commit_graph, gd):
    for i in range(len(topo_ordering)):
        cur_hash = topo_ordering[i]
        cur_node = commit_graph[cur_hash] #current node from topo_sort list, ready to print

        cur_branches = sorted(cur_node.branches)
        if len(cur_branches) == 0: # no branches, simply print hash and that's it
            print(cur_hash)
        else:
            print(cur_hash + " ", end="") # don't want \n
            print(*cur_branches)

        if i < (len(topo_ordering) - 1): # don't handle sticky case if it's the last iteration
            next_hash = topo_ordering[i + 1]
            next_node = commit_graph[next_hash]
            
            parents = {parent.commit_hash for parent in cur_node.parents}
            next_children = {child.commit_hash for child in next_node.children}
            if next_hash not in parents: # print parents and next's children
                print(*parents, end="=\n\n=")
                print(*next_children)

def topo_order_commits():
    git_directory = find_git_directory(os.getcwd()) # git directory if exists
    hash_branch_dict = build_hash_branch_dict(git_directory) # make dictionary of hashes and their branches
    root_commits, commit_graph = build_graphs(git_directory, hash_branch_dict) # build big graph
    topo_ordering = topo_sort(root_commits, commit_graph) # get order from graph
    print_topo(topo_ordering, commit_graph, git_directory) # print order

if __name__ == '__main__':
    topo_order_commits()