#include "Sequence.h"
#include <iostream>
#include <cassert>
using namespace std;

int main() {
	/*Sequence s;
	s.insert("a");
	s.insert("b");
	s.insert("c");
	s.dump();*/
	//Sequence ss;  // ItemType is std::string
	//ss.insert(0, "aaa");
	//ss.insert(1, "ccc");
	//ss.insert(2, "bbb");
	//ItemType x = "xxx";
	//assert(!ss.get(3, x) && x == "xxx");  // x is unchanged
	//assert(ss.get(1, x) && x == "ccc");
	Sequence s;
	// For an empty sequence:
	assert(s.size() == 0);             // test size
	assert(s.empty());                 // test empty
	assert(s.remove("paratha") == 0);  // nothing to remove
}
