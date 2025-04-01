# Description of Work

1. Design Choices
Our overall security.c layout involves some global static data (client hello, server hello, host, keys, etc.),
as well as a switch statement. This switch uses a simple status signal to know what phase we are on (ch, sh, finish,
or general case), which allowed our code to be organized well. When dealing with tlvs, we create the outermost one first,
adding to it incrementally. This modularizes our file, and makes it easier to piece together the data we are building
to send. Our workflow generally followed the diagram guideline in Piazza. To make our solution memory efficient, we 
utilized malloc and free in many areas, as some sizes of data is not known at compile time. Our work heavily used the
API's available in libsecurity.h and consts.h, and we used these whenever possible instead of implementing
from scratch.

2. Problems Encountered and Solutions 
One of the first problems was getting the tlv's sizes accurately. We tried using their length member, but ran into 
issues with this approach. Our solution involved a "dummy call" with a tmp buffer, as calling serialize_tlv with 
the buffer and the tlv would return us the accurate size. Another issue we faced was nondeterministic sizes of
different fields, which led us to having to implement careful memeory management on the heap with malloc and free.
There were tons of tlvs, handshakes, etc. going on, so it was hard to keep track of everything in our heads and 
ensure we were referring to the right thing. To make things clearer, we used very descriptive and detailed 
variable names such as cert_sig_verify_data to mean the data used in the verification of the certificate signature.
In many cases, it was hard to tell what key, function, signature, etc. was supposed to be used. By referring back
to the API documentation, the spec, and Piazza, we could better work through this and piece together the puzzle. 
