#include <cstring>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
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
#define PBIT 0
#define MBIT 0 
#define SBIT 0 
#define TBIT 0 

using namespace std;

struct req_IDs {
	unsigned long ID;
	unsigned long response[6];
	struct req_IDs *next;
};

struct req_IDs *root;


bool cbit = false; // checksum flag
bool request = false; // bit 6 of flag field, 1 = response, 0 = request
unsigned long first_row, req_ID, check_sum, candidate, vote_count, cookie;
unsigned short magic = MAGIC;
char flags, type;
unsigned char out_buffer[24];
unsigned char in_buffer[24];
const unsigned long mask = 0;



// sets first row of data(magic, flags, message type)
void getFirstRow(unsigned short mag, char flag, char msg_type); 

// assigns random 32 bit # to req_ID and stores in vector;
unsigned long getReq_ID();

// place bytes in buffer
void fillBuff();

// vote for candidate
void placeVote();

// query server for voting statistics
void getStats();

// calculate checksum
unsigned long getCheckSum();

// send all bits in buffer
// socket == socket to send data to
// buf == data buffer
// *len == pointer to buffer length in bytes (int)
int sendall(int socket, char *buf, int *len);

int main(int argc, char* argv[])
{
	root = new req_IDs;
	root->next = NULL;
	bool endProg = false;
	bool isValidInput = false;
	cout << "Welcome to the votinator 3000, \n";
	cout << "the program that lets you vote for \n";
	cout << "four billion candidates. Because, yeah.\n\n";
	//get port # from arguments
  unsigned short servPort = atoi(argv[2]);
  //get IP from arguments
  char* IPAddr = argv[1];
  unsigned long servIP;
  //convert dotted decimal address to int
  int status = inet_pton(AF_INET, IPAddr, &servIP);
  cout << "checking IP address\n\n";
  
  // check IP address
  if (status <= 0)
	{
	  cout << "Decimal Address Conversion Error :(\n";
	  exit(-1);
	}else{
		cout << "Valid IP address confirmed\n\n";
	}
  //create a TCP socket
	cout << "Creating TCP socket\n\n";
	
  int sock = socket(AF_INET, SOCK_STREAM,
					IPPROTO_TCP);
  if(sock < 0)
	{
	  cerr << "Socket error :(" << endl;
	  exit(-1);
	}else{
		cout << "Success\n\n";
	}

  //set the fields
  struct sockaddr_in servAddr;

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = servIP;
  servAddr.sin_port = htons(servPort);

  //connect to the server
  cout << "Connecting to server\n\n";
  
  status = connect(sock, (struct sockaddr *) &servAddr,
				   sizeof(servAddr));
  if(status < 0)
	{
	  cerr << "Connection Error." << endl;
	  cout << "Status: " << status << endl;
	  exit(-1);
	}else{
		cout << "Connection successful\n\n";
	}
	
	while(!endProg) {
		while(!isValidInput) {
			int input;
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
		}
	}
	
	return 0;
}

void getFirstRow(unsigned short mag, char flag, char msg_type)
{
	char buffer[4];
	buffer[0] = msg_type;
	buffer[1] = flag;
	buffer[2] = mag & 0xff;
	buffer[3] = (mag >> 8) & 0xff;
	memcpy(&first_row, buffer, sizeof(long));
}

unsigned long getReq_ID()
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
	first_row = htonl(first_row);
	req_ID = htonl(req_ID);
	candidate = htonl(candidate);
	vote_count = htonl(vote_count);
	cookie = htonl(cookie);
	check_sum= getCheckSum();
	out_buffer[23] = (cookie >> 24) & 0xFF;
	out_buffer[22] = (cookie >> 16) & 0xFF;
	out_buffer[21] = (cookie >> 8) & 0xFF;
	out_buffer[20] = (cookie) & 0xFF;
	out_buffer[19] = (vote_count >> 24) & 0xFF;
	out_buffer[18] = (vote_count >> 16) & 0xFF;
	out_buffer[17] = (vote_count >> 8) & 0xFF;
	out_buffer[16] = (vote_count) & 0xFF;
	out_buffer[15] = (candidate >> 24) & 0xFF;
	out_buffer[14] = (candidate >> 16) & 0xFF;
	out_buffer[13] = (candidate >> 8) & 0xFF;
	out_buffer[12] = (candidate) & 0xFF;
	out_buffer[11] = (check_sum >> 24) & 0xFF;
	out_buffer[10] = (check_sum >> 16) & 0xFF;
	out_buffer[9] = (check_sum >> 8) & 0xFF;
	out_buffer[8] = (check_sum) & 0xFF;
	out_buffer[7] = (req_ID >> 24) & 0xFF;
	out_buffer[6] = (req_ID >> 16) & 0xFF;
	out_buffer[5] = (req_ID >> 8) & 0xFF;
	out_buffer[4] = (req_ID) & 0xFF;	
	out_buffer[3] = (first_row >> 24) & 0xFF;
	out_buffer[2] = (first_row >> 16) & 0xFF;
	out_buffer[1] = (first_row >> 8) & 0xFF;
	out_buffer[0] = (first_row) & 0xFF;
}

unsigned long getCheckSum()
{
	return CSUM ^ first_row ^ req_ID ^ candidate ^ vote_count ^ cookie;
}

void placeVote()
{
	//set first row bytes -- magic = MAGIC, flags = 0x18, type = TYPEINQ
		magic = MAGIC;
		flags = (char) 0x18;
		type = TYPEINQ;
		getFirstRow(magic, flags, type);
		req_ID = getreq_ID();
		vote_count = 0x0;
		cookie = 0x0;
		
	cout << "Please enter a candidate number between 0x0 and 0xFFFFFFFF: ";
	cin >> hex >> candidate;
	cout << "candidate: " << candidate << endl;
}

void getStats()
{
	//set first row bytes -- magic = MAGIC, flags = 0x08, type = TYPEINQ
		magic = MAGIC;
		flags = (char) 0x08;
		type = TYPEINQ;
		getFirstRow(magic, flags, type);
		req_ID = rand();
		candidate = 0x0;
		vote_count = 0x0;
		cookie = 0x0;
				
}

int sendall(int socket, char *buf, int *len)
{
		int total = 0;
		int bytesleft = *len;
		int n;
		
		while(total < *len) {
			n = send(socket, buf+total, bytesleft, 0);
			if (n == -1) {break; }
			total += n;
			bytesleft -= n;
		}

		*len = total;
		return n == -1?-1:0; // return -1 on failure, 0 on success
}
