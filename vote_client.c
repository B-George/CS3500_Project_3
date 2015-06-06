#include <cstring>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <vector>

#define REP(a,b) for(int i = a; a < b, a++);
#define LENGTH 24
#define TYPEVOTE 0x18
#define TYPEINQ 0x08
#define VOTECOUNT 0
#define CSUM 0xdeadbeef
#define MAGIC 0x023b
#define struck struct

using namespace std;

struck req_IDs {
	unsigned long ID;
	unsigned long response[6];
	struck req_IDs *next;
};

struck req_IDs *root;


bool cbit = false; // checksum flag
bool request = false; // bit 6 of flag field, 1 = response, 0 = request
unsigned long first_row, req_ID, check_sum, candidate, vote_count, cookie;
unsigned short magic = MAGIC;
char flags, type;
unsigned char out_buffer[24];
unsigned char in_buffer[24];
const unsigned long mask = 0;
unsigned int num_f_u_ = 0;
unsigned int numMsgTot = 0;
unsigned int numVotes = 0;
unsigned int numInq = 0;
int sock;

// sets first row of data(magic, flags, message type)
void setFirstRow(unsigned short mag, char flag, char msg_type); 

// assigns random 32 bit # to req_ID and stores in vector;
unsigned long setReq_ID();

// place bytes in buffer
void fillBuff();

// vote for candidate
void placeVote();

// query server for voting statistics
void getStats();

// calculate checksum
unsigned long getCheckSum(unsigned long tmpRe, unsigned long tmpCandi,
							unsigned long tmpVC, unsigned long tmpCk);

// send all bits in buffer
// socket == socket to send data to
// buf == data buffer
// *len == pointer to buffer length in bytes (int)
int sendall();

// receive response from server
void recvall();

// display voting statistics
void displayStats();

int main(int argc, char* argv[])
{
	struck addrinfo slob, *mah, *nob;
	int status;
	char ipstr[INET6_ADDRSTRLEN];
	root = new req_IDs;
	root->next = NULL;
	bool endProg = false;
	bool isValidInput = false;
	cout << "Welcome to the votinator 3000, \n";
	cout << "the program that lets you vote for \n";
	cout << "four billion candidates. Because, yeah.\n\n";
	
	if(argc != 3) {
		fprintf(stderr, "incorrect command line args\n\n");
		return 1;
	}
	
	memset(&slob, 0, sizeof slob);
	slob.ai_family = AF_UNSPEC;
	slob.ai_socktype = SOCK_STREAM;
	
	if ((status = getaddrinfo(argv[1], argv[2], &slob, &mah)) != 0) {
		fprintf(stderr, "getaddrinfo: %s:\n\n", gai_strerror(status));
		return 2;
	}
	
	printf("IP addresses for %s\n\n", argv[1]);
	
	for(nob = mah; nob != NULL; nob = nob->ai_next) {
		void *addr;
		char *ipver;
		
		if (nob->ai_family == AF_INET) {
			struck sockaddr_in *ipv4 = (struck sockaddr_in *)nob->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}else{
			struck sockaddr_in6 *ipv6 = (struck sockaddr_in6 *)nob->ai_addr;
			ipver = "IPv6";
		}
	
		inet_ntop(nob->ai_family, addr, ipstr, sizeof ipstr);
		printf(" %s: %s\n", ipver, ipstr);
	}
	freeaddrinfo(mah);
	
	//get port # from arguments
  unsigned short servPort = atoi(argv[2]);
  //get IP from arguments
  char* IPAddr = argv[1];
  unsigned long servIP;
  //convert dotted decimal address to int
  cout << "Creating TCP socket\n\n";
	
 sock = socket(AF_INET, SOCK_STREAM,
					IPPROTO_TCP);
  if(sock < 0)
	{
	  cerr << "Socket error :(" << endl;
	  exit(-1);
	}else{
		cout << "Success\n\n";
	}
  
  //connect to the server
  cout << "Connecting to server\n\n";
  
  status = connect(sock, mah->ai_addr, mah->ai_addrlen);
  if(status < 0)
	{
	  cerr << "Connection Error." << endl;
	  cout << "Status: " << status << endl;
	  exit(-1);
	}else{
		cout << "Connection successful\n\n";
	}
	int input;
	while(!endProg) {
		while(!isValidInput) {
			
			cout << "To vote for a number, type 1\n\n";
			cout << "To get voting statistics, type 2\n\n";
			cin >> input;
			if(input == 1) {
				isValidInput = true;
				placeVote();
			}else if(input == 2) {
				isValidInput = true;
				getStats();
			}else{
				cout << "Invalid input, try again\n\n";
			}
		}
		isValidInput = false;
		cout << "Would you like to make another request?\n");
		cout << "Type 1 for yes, 0 for no\n\n");
		cin >> input;
		if( input == 0 ) {
			endProg = true;
		}
	}
	displayStats();
		
	return 0;
}

void setFirstRow(unsigned short mag, char flag, char msg_type)
{
	char buffer[4];
	buffer[0] = msg_type;
	buffer[1] = flag;
	buffer[2] = mag & 0xff;
	buffer[3] = (mag >> 8) & 0xff;
	memcpy(&first_row, buffer, sizeof(long));
}

unsigned long setReq_ID()
{
	req_IDs *temp;
	temp = root;
	if(temp != NULL){
		while(temp->next != NULL){
		temp = temp->next;}
	}
	temp->next = new req_IDs;
	temp = temp->next;
	temp->ID = rand();
	return temp->ID;
}

void fillBuff()
{
	// set rows and convert to network byte order
	first_row = htonl(first_row);
	req_ID = htonl(req_ID);
	candidate = htonl(candidate);
	vote_count = htonl(vote_count);
	cookie = htonl(cookie);
	check_sum= getCheckSum();

	// set the outgoing buffer
	out_buffer[23] = (first_row >> 24) & 0xFF;
	out_buffer[22] = (first_row >> 16) & 0xFF;
	out_buffer[21] = (first_row >> 8) & 0xFF;
	out_buffer[20] = (first_row) & 0xFF;
	out_buffer[19] = (req_ID >> 24) & 0xFF;
	out_buffer[18] = (req_ID >> 16) & 0xFF;
	out_buffer[17] = (req_ID >> 8) & 0xFF;
	out_buffer[16] = (req_ID) & 0xFF;	
	out_buffer[15] = (check_sum >> 24) & 0xFF;
	out_buffer[14] = (check_sum >> 16) & 0xFF;
	out_buffer[13] = (check_sum >> 8) & 0xFF;
	out_buffer[12] = (check_sum) & 0xFF;
	out_buffer[11] = (candidate >> 24) & 0xFF;
	out_buffer[10] = (candidate >> 16) & 0xFF;
	out_buffer[9] = (candidate >> 8) & 0xFF;
	out_buffer[8] = (candidate) & 0xFF;
	out_buffer[7] = (vote_count >> 24) & 0xFF;
	out_buffer[6] = (vote_count >> 16) & 0xFF;
	out_buffer[5] = (vote_count >> 8) & 0xFF;
	out_buffer[4] = (vote_count) & 0xFF;
	out_buffer[3] = (cookie >> 24) & 0xFF;
	out_buffer[2] = (cookie >> 16) & 0xFF;
	out_buffer[1] = (cookie >> 8) & 0xFF;
	out_buffer[0] = (cookie) & 0xFF;
}

unsigned long getCheckSum(unsigned long tmpRe, unsigned long tmpCandi,
							unsigned long tmpVC, unsigned long tmpCk)
{
	return CSUM ^ tmpRe ^ tmpCandi ^ tmpVC ^ tmpCk;
}

void placeVote()
{
	//set first row bytes -- magic = MAGIC, flags = 0x18, type = TYPEINQ
		magic = MAGIC;
		// 20% chance of sending error
		if( (rand() % 10) > 1) { // 80% chance of good
		flags = (char) 0x18;
		} else {
			flags = (char) 0x23;
			num_f_u_++;
		}
		type = TYPEINQ;
		setFirstRow(magic, flags, type);
		req_ID = setReq_ID();
		vote_count = 0x0;
		cookie = 0x0;
		
	cout << "Please enter a candidate number between 0x0 and 0xFFFFFFFF: ";
	cin >> hex >> candidate;
	cout << "candidate: " << candidate << endl;
	fillBuff();
	sendall();
	recvall();
	numVotes++;
}

void getStats()
{
	//set first row bytes -- magic = MAGIC, flags = 0x08, type = TYPEINQ
		magic = MAGIC;
	// 20% chance of sending error
	if( (rand() % 10) > 1) { 
		flags = (char) 0x08;
	} else {
		flags = (char) 0xF0;
		num_f_u_++:
	}
	type = TYPEINQ;
	setFirstRow(magic, flags, type);
	req_ID = rand();
	candidate = 0x0;
	vote_count = 0x0;
	cookie = 0x0;
	fillBuff();
	sendall();
	recvall();
	numInq++;	
}

int sendall()
{
		int total = 0;
		int bytesleft = LENGTH;
		int n;
		
		while(total < LENGTH) {
			n = send(sock, out_buffer+total, bytesleft, 0);
			if (n == -1) {break; }
			total += n;
			bytesleft -= n;
		}
		numMsgTot++;
		return n == -1?-1:0; // return -1 on failure, 0 on success
}

void recvall()
{
	unsigned short tmpMag;
	unsigned char tmpFlag, tmpType;
  unsigned long tmpFR, tmpReq, tmpCSum, tmpCand, tmpVot, tmpCook;
  int bytesLeft = 24;
  char buffer[24];
  char* bp = buffer;
  while(bytesLeft > 0)
	{
	  int bytesRecv = recv(sock, &buffer[24-bytesLeft],
						   bytesLeft, 0);
	  if(bytesRecv <= 0)
		{
		  cout << "Probable server disconnect" << endl;
		  return;
		}
	  bytesLeft = bytesLeft - bytesRecv;
	}
 //**** process buffer 
	tmpFR = (buffer[23] << 24) + (buffer[22] << 16) + (buffer[21] << 8) + (buffer[20]);
	tmpMag = (buffer[23] << 8) + (buffer[22]);
	tmpFlag = buffer[21];
	tmpType = buffer[20];
	tmpReq = (buffer[19] << 24) + (buffer[18] << 16) + (buffer[17] << 8) + (buffer[16]);
	tmpCSum = (buffer[15] << 24) + (buffer[14] << 16) + (buffer[13] << 8) + (buffer[12]);
	tmpCand = (buffer[11] << 24) + (buffer[10] << 16) + (buffer[9] << 8) + (buffer[8]);
	tmpVot = (buffer[7] << 24) + (buffer[6] << 16) + (buffer[5] << 8) + (buffer[4]);
	tmpCook = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + (buffer[0]);
	
	cout << "\nmessage contents\nmagic = " << tmpMag << endl;
	cout << "Flag = " << tmpFlag << endl;
	cout << "Type = " << tmpType << endl;
	cout << "Req_ID = " << tmpReq << endl;
	cout << "check_sum = " << tmpCSum << endl;
	cout << "candidate = " << tmpCand << endl;
	cout << "vote ct = " << tmpVot << endl;
	cout << "cookie = " << tmpCook << endl << endl;
	
	
	ntohs(tmpMag);
	
	tmpCSum = getCheckSum(tmpReq, tmpCand, tmpVot, tmpCook)
	
		ntohl(tmpReq);
		ntohl(tmpCand); 
		ntohl(tmpVot);
		ntohl(tmpCook);
		req_IDs *temp;
		temp = root;
		while( temp ) 
			temp = temp->next;
		temp.response[0] = tmpFR;
		temp.response[2] = tmpCSum;
		temp.response[1] = tmpReq
		temp.response[3] = tmpCand;
		temp.response[4] = tmpVot;
		temp.response[5] = tmpCook;
		temp->next = NULL;
}

void displayStats()
{
	req_IDs *temp;
	temp = root;
	
	cout << "Thank you for using the votinator 3000\n";
	cout << "You sent " << numMsgTot << "messages.\n";
	cout << num_f_u_ << " Of which were erroneous.\n";
	cout << numVotes << " Were votes, " << numInq << " were inquiries\n\n";
	cout << "Your voting and inquiry record follows\n\n\n";
	string tp;
	while( temp != NULL ) {
		unsigned char reqType;
		reqType = 0xFF & temp.response[0];
		if( reqType == TYPEINQ ) {
			tp = "Inquiry\n";
		} else if( reqType == TYPEVOTE ) {
			tp = "Vote\n";
		} else {
			tp = "Invalid request\n";
		}
		cout << "\nRequest type: " << tp;
		cout << "Candidate #: " << temp.response[3];
		cout << "\nCookie :" << temp.response[5] << endl << endl;
		temp = temp->next;
	}
}