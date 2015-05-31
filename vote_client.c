#include <cstring>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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

bool cbit = false; // checksum flag
bool request = false; // bit 6 of flag field, 1 = response, 0 = request
unsigned long first_row, req_ID, check_sum, candidate, vote_count, cookie;
unsigned short magic = MAGIC;
char flags, type;
char *msg[24];
const unsigned long mask = 0;

// sets first row of data(magic, flags, message type)
void getFirstRow(unsigned short mag, char flag, char msg_type); 

// vote for candidate
void placeVote();

// query server for voting statistics
void getStats();

// calculate checksum
unsigned long getCheckSum();

int main(int argc, char* argv[])
{
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
  cout << "checking IP address";
  delay(100);
  cout << ".";
  delay(100);
  cout << ".";
  delay(100);
  cout << ".\n";
  
  // check IP address
  if (status <= 0)
	{
	  cout << "Decimal Address Conversion Error :(\n";
	  exit(-1);
	}else{
		cout << "Valid IP address confirmed\n\n";
	}
  //create a TCP socket
	cout << "Creating TCP socket";
	delay(100);
	cout << ".";
	delay(100);
	cout << ".";
	delay(100);
	cout << ".\n";
	
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
  cout << "Connecting to server";
  delay(100);
  cout << ".";
  delay(100);
  cout << ".";
  delay(100);
  cout << ".\n";
  
  status = connect(sock, (struct sockaddr *) &servAddr,
				   sizeof(servAddr));
  if(status < 0)
	{
	  cerr << "Connection Error :(" << endl;
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
			if((input == 1) || (input == 1) {
				isValidInput = true;
			}else{
				cout << "Invalid input, try again\n\n";
			}
		}
		isValidInput = false;
		switch (input) {
			case 1:
				placeVote();				
			case 2:
				getStats();
		}
	}

	//change something
	
	
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

unsigned long getCheckSum()
{
	return CSUM ^ first_row ^ req_ID ^ candidate ^ vote_count ^ cookie;
}

void placeVote()
{
	cout << "Please enter a candidate number between 0x0 and 0xFFFFFFFF: ";
	cin >> hex >> candidate;
	cout << "candidate: " << candidate << endl;
}

void getStats()
{
	//set first row bytes -- magic = MAGIC, flags = 0x80, type = TYPEINQ
		magic = MAGIC;
		flags = (char) 0x80;
		type = TYPEINQ;
		getFirstRow(magic, flags, type);
		req_ID = rand();
		candidate = 0x0;
		vote_count = 0x0;
		cookie = 0x0;
		check_sum = getCheckSum();
		
}