# CS3500_Project_3
Simple client/servier voting program designed as follows

CPSC 3500 Computing Systems
Project 2: iVote – Voting Protocol
Due: 11:59PM, Wednesday, June 3, 2015
Friday, June 5, 2015
1. Project Overview
For this project, you will implement the iVote Protocol, a simple client-server voting protocol that allows
clients to "vote" for candidates by sending messages to a server. The server replies to every vote message
containing the number of votes cast so far for the given candidate. Users can also inquire, without casting
a vote, about the number of votes cast so far for any particular candidate.
For added interest, the protocol allows clients to vote any number of times, for any number of candidates.
Each candidate is identified (only) by a 32-bit integer; thus there can be up to 232 different candidates.
The voting protocol runs over TCP. For this project you will implement both client and server sides of
the Voting protocol.
You will first test your own client and server against each other. Once you are satisfied with that your
client and server work with each other, you may test them against the other group’s implementation.
The rest of this document specifies the Voting protocol and provides some small hints for implementation.
2. Message Formats
The request sent by the client and the response sent back by the server have the same format. Each
message is exactly 24 bytes long. The part of the format that is common to all messages is shown below.
All multi-byte integers are sent in network (big-endian) byte order.
 |----16 bits ----|------16 bits---|
 -----------------------------------
 | Magic | Flags | Type |
 -----------------------------------
 | Request ID |
 -----------------------------------
 | Checksum |
 -----------------------------------
 | 12 bytes of |
 | Type-specific |
 | Info |
 -----------------------------------
The "Magic" field (16 bits) always contains the value 571 (hexadecimal 0x023B), in big-endian order.
That is, the first byte contains 2, the second byte contains (decimal) 59. This provides both a version
number and a bit of redundancy as a sanity-check.
The "Flags" field (8 bits) contains six flags, two of which indicate functionality, and four of which
indicate errors.
 7 6 5 4 3 2 1 0
 -----------------
 |C|R|-|-|P|M|S|T|
 -----------------
 The C bit (the most significant bit of the third byte of the message) indicates whether the sender
computed the checksum before sending the message. That is, the checksum can be ignored in any
received message with the C bit equal to 0. The receiver MUST verify the checksum of any
message with the C bit equal to 1.
 The R bit (bit 6) indicates whether the message is a Request or Response. (1=Response).
 Bits 4 and 5 are reserved. Their value is ignored in any received message; they should be set to
zero in both request and response.
 The low-order four bits of the flags byte are the error indication bits. They are set by the server in
the response sent for an erroneous Request message, and provide information about the errors
found in the Request. (These bits MUST all be set to 0 in a request.) The meaning assigned to
each error bit is as follows:
o The P bit (bit 3) is set if the R flag was set in a request (i.e. a packet sent to the server).
o The M bit (bit 2) is set if the magic number was wrong in the Request.
o The S bit (bit 1) is set if the checksum flag was set and but checksum was incorrect (see
below for the details of checksum verification).
o The T bit (bit 0) is set if the "Type" field is invalid, i.e. contains an unrecognized type.
Note that these bits are independent, i.e. more than one can be set at a time.
The fourth byte of the message is the Type field, which determines how the rest of the information is
interpreted. The type field contains a number identifying the type of the message. Valid types are:
 Vote Request/Response (Type = 0x18): the request contains a candidate number, for which the
client is registering a vote. The response contains the same number, plus the vote count for that
candidate (including the just-recorded vote), plus a "cookie" (32-bit number) that serves as a
"receipt" for the vote.
 Vote Count Inquiry Request/Response (Type = 0x08): the request contains a candidate number,
and server returns a response containing the vote count for that candidate, which is not changed
by the transaction.
The next field (bytes 5-8) is the Request ID (32 bits). The Request ID field is set by the client to any
value of the client's choosing. It is ignored by the server, and is returned unchanged in the response. Thus,
the client can use this field to match responses to requests (e.g. if the client sends multiple requests
without waiting for a response, it can assign a different Request ID to each one and use them to match the
responses with the requests). 
The final common field is the Checksum, which is 32 bits. If the message is protected by a checksum, the
checksum flag (C bit, bit 0) MUST be set in the Flags field. In that case, the sender computes the
checksum as described below, and the receiver validates it also as described below. If the message is not
protected by a checksum, the sender MUST set the C bit to 0 and place the value 0 in the checksum field.
(The ability to turn off the checksum protection is sometimes useful for debugging.)
The following sections describe details of each particular type of message.
2.1 Vote Message
The complete format of the Vote message is shown below.
 -----------------------------------
 | Magic | Flags | Type |
 -----------------------------------
 | Request ID |
 -----------------------------------
 | Checksum |
 -----------------------------------
 | Candidate # |
 -----------------------------------
 | Vote Count |
 -----------------------------------
 | Cookie |
 -----------------------------------
The Vote message is identified by the value 24 (decimal) in the Type field.
The Candidate Number field is a 32-bit unsigned integer, in network (big-endian) byte order.
(Candidates are not identified by any means other than numbers. It makes the advertisements much
shorter.) The value of this field is the same in the response as in the request.
Following the Candidate Number is the Vote Count for the candidate, a 32 bit unsigned integer. The
client SHOULD set this field to 0 in a request. In the server response, this field is set to the current
number of votes received for the indicated candidate ONLY if none of the error bits are set. If no error bits
are set, the count includes the vote cast by the request message. Thus, in a response, either some error flag
is set, or the vote count is at least 1.
The final field is the 32-bit Cookie field. It SHOULD contain 0 in a Request. In every response it
contains a random value that the server also logs. Thus, the cookie serves as a receipt for the client.
2.2 Vote Inquiry Message
The Vote Inquiry message has the value 8 in the Type field. Otherwise it has exactly the same format as
the Vote message. The only difference in semantics is that the Vote Count field in the response contains
the number of votes tallied for the specified candidate at the time the message is received. Thus, the
returned value may be zero. 
3. Checksum Computation
The sender performs the following computation as the last step before transmitting the message (i.e.
AFTER fields have been put in network byte order).
1. Place the value 0xdeadbeef (decimal 3735928559) in the checksum field in big-endian order, that
is 0xde, 0xad, 0xbe, 0xef.
2. Compute the bitwise exclusive-or of all the 32-bit words in the message (the "^" operator in
C/C++).
3. Write the result into the checksum field.
The receiver performs the following computation on every received message that has the checksum flag
set: compute the bitwise exclusive-or of all 32-bit words in the message. If the result is not 0xdeadbeef in
network byte order, discard the message: it is erroneous. Otherwise, deliver the message. (Note: the server
running on cs1 does not discard messages with erroneous checksums; instead, it sets the "Checksum
error" flag and returns a response anyway.)
Implementation note: To implement this you should write a C/C++ function compute_csum(unsigned
int *msg) that computes the bitwise XOR of six 32-bit words, starting at the given address.
4. Example
To register a vote for candidate number 23, a client might send the message below:
 -----------------------------------
 |0000000111010111|0000000000011000| Flags = none, Type = 0x18
 -----------------------------------
 |0000000000000000 0000000000000010| Request ID = 2
 -----------------------------------
 |0000000000000000 0000000000000000| Checksum = 0
 -----------------------------------
 |0000000000000000 0000000000010111| Candidate # = 23
 -----------------------------------
 |0000000000000000 0000000000000000| Vote Count = 0
 -----------------------------------
 |0000000000000000 0000000000000000| Cookie = 0
 -----------------------------------
If Candidate 23 had received 126 votes before the client's request, the server would send the reply:
 -----------------------------------
 |0000000111010111|1100000000011000| Flags = C,R; Type = 0x18
 -----------------------------------
 |0000000000000000 0000000000000010| Request ID = 2
 -----------------------------------
 |1010001001000011 1101011110100101| Checksum = even parity per bit
 -----------------------------------
 |0000000000000000 0000000000010111| Candidate # = 23
 -----------------------------------
 |0000000000000000 0000000001111111| Vote Count = 127
 -----------------------------------
 |1010001110010100 0001011111010111| Cookie = random #
 -----------------------------------
5. Protocol operations
The server receives messages on the specified port. When it receives a message, it checks it thoroughly
for well-formedness. If the message is well-formed, it carries out the requested function: retrieving the
vote count for a given candidate and incrementing it if it's a vote message. The server then constructs a
response message containing the results and sends it back to the client (i.e., to the IP address and port
from which the request originated).
If the message is malformed in some way, the server takes the original message, turns on the appropriate
error flags and the response bit, recalculates the checksum, and sends it back to the client.
6. Implementation hints
Your server needs to keep track of vote counts for up to several hundred candidates. The most efficient
way to do this is via hash table, but you may implement it any way you want.
It is strongly suggested that you separate the parsing of input messages from the other functionality of the
program. That is, write a separate function or method that takes a buffer as argument, and returns a data
structure containing the information contained in the message, or an indication of any errors that occur in
parsing.
Remember to code defensively. Your server must be prepared for malformed messages (especially when
your server may receive malformed messages from other groups’ clients during competition). Before
attempting to parse an incoming message, be sure to check that it contains enough data to be a legitimate
message!
7. Your Job
7.1 Form the team
You should form the team and inform the instructor as soon possible. Then you will get a group ID which
could be used for port number resource assignment! Each team can consist of a max of 3 members.
7.2 Implement a client
 The client can allow a user to send Vote and Vote Inquiry requests for a candidate whose IDs are
input by the user.
 The client should interpret and display the responses to its requests.
 The needs to model a misbehaving client. That is, it with certain probability (i.e., p = 0.2) issues
malformed messages. Please refer to Section 2 Message Formats for details.
 The client can be designed as menu-based program.
 After the client program terminates, it should print out statistics in past activities: such as # of
vote messages with cookie #, # of inquiry messages (for which candidate), # of total messages,
and # of malformed messages. Regarding the statistics, the more detailed the better!
 The client should look like (make the server name and port number as command-line arguments):
./vote_client [server name] [port #]
7.3 Implement a server
 The server can process Vote and Vote Inquiry requests and send correct responses (including
detection of errors and setting the appropriate error flags).
 The server needs to be multi-threading program or use non-blocking select(), and thus is able to
handle concurrent client connections. Handle race conditions if there is any! Use your client from
the first part to test your server.
 When the server terminates, it should print out the statistics of the past activities: # of vote
messages, # of inquiry messages, # of malformed requests, # of votes for each candidates.
Regarding the statistics, the more detailed the better!
7.4 Following the protocol
Your client/server program will work for other teams’ client/server program if your protocol
implementation correctly follows the descriptions. If time allows, we may have group completion to test
the program against each other.
The grading of the project also will test your program against other teams’ program. MAKE SURE
THAT YOU FOLLOW THE PROTOCOL!
8. Port numbers
The port number used in the project must be between 10000 – 20000. If your group ID is k, then
the ports assigned to you is between 10000 + k * 10 and 10000 + (k + 1)*10 – 1.
9. Submission
 All source code files
 A Makefile file
 Readme: list highlights of your design & implementation as well as limitations, and each
team member’s contribution.
 Use tar to pack all files above into a package named project3.tar
 Submission command (you can submit at the server cs1 multiple times before deadline;
only the most recent submission is kept):
/home/fac/testzhuy/CPSC3500/upload p3 project3.tar
