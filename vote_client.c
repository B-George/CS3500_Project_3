#include <cstring>
#include <cstdlib>
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

// calculate checksum
unsigned long getCheckSum();

int main(int argc, char* argv[])
{
	
	//get port # from arguments
  unsigned short servPort = atoi(argv[2]);
  //get IP from arguments
  char* IPAddr = argv[1];
  unsigned long servIP;
  //convert dotted decimal address to int
  int status = inet_pton(AF_INET, IPAddr, &servIP);
  if (status <= 0)
	{
	  cout << "Decimal Address Conversion Error :(\n";
	  exit(-1);
	}
  //create a TCP socket

  int sock = socket(AF_INET, SOCK_STREAM,
					IPPROTO_TCP);
  if(sock < 0)
	{
	  cerr << "Socket error :(" << endl;
	  exit(-1);
	}

  //set the fields
  struct sockaddr_in servAddr;

  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = servIP;
  servAddr.sin_port = htons(servPort);

  //connect to the server
  status = connect(sock, (struct sockaddr *) &servAddr,
				   sizeof(servAddr));
  if(status < 0)
	{
	  cerr << "Connection Error :(" << endl;
	  cout << "Status: " << status << endl;
	  exit(-1);
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

unsigned long getCheckSum()
{
	return CSUM ^ first_row ^ req_ID ^ candidate ^ vote_count ^ cookie;
}
