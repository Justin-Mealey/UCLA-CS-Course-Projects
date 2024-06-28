#include "Sequence.h"

#include <iostream>
using namespace std;

Sequence::Sequence() {
	m_size = 0;
	head = nullptr;
	tail = nullptr;
}
Sequence::~Sequence() {
	for (head; head != nullptr; head = head) {
		Node* p = head;
		head = head->next;
		delete p;
	}
}
Sequence::Sequence(const Sequence& other) {
	m_size = other.m_size;
	const int tempsize = m_size; //for when we loop, want it constant
	if (m_size == 0) { //empty sequence
		head = nullptr;
		tail = nullptr;
	}
	else {
		for (int i = 0; i < tempsize; i++) { //loop over all positions
			if (i == 0) { //1st iteration need to to the empty() if statement in insert, so m_size must be 1
				m_size = 0;
			}
			ItemType othersvalue;
			other.get(i, othersvalue); //get value from other
			insert(i, othersvalue); //plug it into this
		}
	}
}
Sequence& Sequence::operator=(const Sequence& rhs) {
	if (this != &rhs) { //if not the same
		Sequence temp(rhs); //make a temp
		swap(temp); //swap temp and this, so this now is a copy of rhs but doesnt have same address
	}
	return *this;
}

bool Sequence::empty() const {
	if (m_size == 0)
		return true;
	return false;
}
int Sequence::size() const {
	return m_size;
}
int Sequence::insert(int pos, const ItemType& value) {
	if (0 <= pos && pos <= size()) { //if valid position
		if (empty()) { //make our first node
			head = new Node;
			tail = head;
			head->next = nullptr;
			head->prev = nullptr;
			head->data = value;
			m_size++;
			return pos;
		}
		else if(pos == 0){ //make new node, change head
			Node* newhead = new Node;
			newhead->next = head;
			newhead->prev = nullptr;
			newhead->data = value;
			head->prev = newhead;
			head = newhead;
			m_size++;
			return pos;
		}
		else if (pos == size()) { //make new node, change tail
			Node* newtail = new Node;
			newtail->next = nullptr;
			newtail->prev = tail;
			newtail->data = value;
			tail->next = newtail;
			tail = newtail;
			m_size++;
			return pos;
		}
		else { //make new node, connect it to previous and next nodes
			Node* p = head;
			for (int i = 0; i < pos; i++) { //make p point to the node at the position we want to insert
				p = p->next;
			}
			Node* newnode = new Node;
			newnode->data = value;
			newnode->next = p;
			newnode->prev = p->prev; 
			p->prev->next = newnode; //the node before new node must point to new node
			p->prev = newnode;       //p is after newnode, so p->prev is newnode
			m_size++;
			return pos;
		}
	}
	return -1;
}
int Sequence::insert(const ItemType& value) {
	Node* p = head;
	int pos;
	for (pos = 0; pos < size(); pos++) { //loop over sequence trying to find when value <= data
		if (value <= p->data) { //when we find it break and insert at this position
			break;
		}
		if (p->next == nullptr) { //if we couldn't find it, insert at the end
			return insert(size(), value);
		}
		p = p->next; //each iteration move p to next node
	}
	return insert(pos, value);
}
bool Sequence::erase(int pos) {
	if (0 <= pos && pos < size()) {
		if (pos == 0) { 
			if (size() == 1) { //erasing only item, now it should be empty
				head = nullptr;
				tail = nullptr;
				m_size--;
				return true;
			}
			else { //moving head to next node and erasing old head
				Node* temp = head;
				head = head->next;
				head->prev = temp->prev;
				delete temp;
				m_size--;
				return true;
			}
		}
		else if (pos == size() - 1) { //if erasing last move tail up one node and erase last node.
			Node* temp = tail;
			tail = tail->prev;
			tail->next = temp->next;
			delete temp;
			m_size--;
			return true;
		}
		else { //connect previous and next nodes, delete node
			Node* p = head;
			for (int i = 0; i < pos; i++) {
				p = p->next;
			}
			p->prev->next = p->next;
			p->next->prev = p->prev;
			delete p;
			m_size--;
			return true;
		}
	}
	else
		return false;
}
int Sequence::remove(const ItemType& value) {
	int counter = 0;
	Node* p = head;
	for (int i = 0; i < size(); i++) {//loop over sequence
		if (p == nullptr) { //at end of sequence, break and return
			break;
		}
		if (p->data == value) { //if its a match
			p = p->next; //move p ahead (we're about to erase where p is at)
			erase(i);
			i--; // when we erase, everything after moves left one position, this accounts for that
			counter++;
		}
		else {
			p = p->next;
		}
	}
	return counter;
}
bool Sequence::get(int pos, ItemType& value) const {
	if (0 <= pos && pos < size()) {
		Node* p = head;
		for (int i = 0; i < pos; i++) { //make p point to the node we want to get data from
			p = p->next;
		}
		value = p->data;
		return true;
	}
	return false;
}
bool Sequence::set(int pos, const ItemType& value) {
	if (0 <= pos && pos < size()) {
		Node* p = head;
		for (int i = 0; i < pos; i++) { //make p point to the node we are changing
			p = p->next;
		}
		p->data = value;
		return true;
	}
	return false;
}
int Sequence::find(const ItemType& value) const {
	Node* p = head;
	for (int i = 0; i < size(); i++) {
		if (p->data == value) {
			return i;
		}
		p = p->next;
	}
	return -1;
}
void Sequence::swap(Sequence& other) {
	int tempsize = m_size;
	m_size = other.m_size;
	other.m_size = tempsize;
	Node* temphead = head;
	head = other.head;
	other.head = temphead;
	Node* temptail = tail;
	tail = other.tail;
	other.tail = temptail;
}

int subsequence(const Sequence& seq1, const Sequence& seq2) {
	if (seq2.empty() || seq2.size() > seq1.size()) { //if s2 empty or s2size ? s1size, we wont have a subsequence
		return -1;
	}
	else {
		ItemType valOf2;
		int k = -1; //initialize to bad value
		for (int i = 0; i < seq1.size(); i++) {
			ItemType valOf1;
			seq1.get(i, valOf1);
			seq2.get(0, valOf2);
			if (valOf2 == valOf1) { //change k to postion where head of s2 = s1 at position
				k = i;
				if (i + seq2.size() > seq1.size()) { //safety conditional to not overstep past s1's elements
					return -1;
				}
				bool isSubsequence = true; //initialize true, set to false if we prove its not a subsequence
				for (int j = 1; j < seq2.size(); j++) { //loop over remaining elements of s2, ensuring s2val = s1val
					seq2.get(j, valOf2);
					seq1.get(i + j, valOf1);
					if (valOf1 != valOf2) { //if not same set false
						isSubsequence = false;
					}
				}
				if (isSubsequence) //if still true, its a subsequence so we return, if not we go to next loop iteration
					return k;
			}
		}
		return -1; //at this point k could be anything, but if we haven't already returned k, there are no subsequences
	}
}
void concatReverse(const Sequence& seq1, const Sequence& seq2, Sequence& result) {
	Sequence temp;
	int index = 0;
	for (int i = seq1.size() - 1; i >= 0; i--) { //copy values (in reverse) of s1 to temp
		ItemType copyvalue;
		seq1.get(i, copyvalue);
		temp.insert(index, copyvalue);
		index++;
	}
	for (int i = seq2.size() - 1; i >= 0; i--) { //copy values (in reverse) of s2 to temp
		ItemType copyvalue;
		seq2.get(i, copyvalue);
		temp.insert(index, copyvalue);
		index++;
	}
	result = temp; // use assignment operator to change result to whatever it needs to be
}




void Sequence::dump()const{
	Node* tracer = head;
	if (tracer == nullptr) {
		cerr << "Sequence is empty" << endl;
		cerr << "Returning" << endl << endl;
		return;
	}
	for (int i = 0; i < size(); i++) {
		cerr << "Value at pos " << i << " = " << tracer->data << endl;
		if (tracer->next == nullptr) {
			cerr << "End of list" << endl;
			cerr << "head->data = " << head->data << ", tail->data = " << tail->data << endl;
			cerr << "size is " << size() << endl;
			cerr << "Returning" << endl << endl;
			return;
		}
		tracer = tracer->next;
	}
}