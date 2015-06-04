// Benjamin George and Eli Gatchalian
//
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
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>

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


// globals
struct req_IDs *root;

bool cbit = false; // checksum flag
bool request = false; // bit 6 of flag field, 1 = response, 0 = request
unsigned long first_row, req_ID, check_sum, candidate, vote_count, cookie;
unsigned short magic = MAGIC;
char flags, type;
unsigned char out_buffer[24];
unsigned char in_buffer[24];
const unsigned long mask = 0;

pthread_mutex_t boardLock;

struct ThreadArgs
{
  int clientSock;
};

// sets first row of data(magic, flags, message type)
void getFirstRow(unsigned short mag, char flag, char msg_type); 

// place bytes in buffer
void fillBuff();

// calculate checksum
unsigned long getCheckSum();

// send all bits in buffer
// socket == socket to send data to
// buf == data buffer
// *len == pointer to buffer length in bytes (int)
int sendall(int socket, char *buf, int *len);



//Send a long over the socket
int sendLong(int clientSock, long num);

//Receive a long over the socket
int recvLong(int clientSock, long &msg);

//The main client-server interaction loop
void processClient(int clientSock)
{
  
  int bytesLeft = 24;
  char buffer[24];
  char* bp = buffer;
  while(bytesLeft > 0)
	{
	  int bytesRecv = recv(clientSock, &buffer[24-bytesLeft],
						   bytesLeft, 0);
	  if(bytesRecv <= 0)
		{
		  cout << "Probable client disconnect" << endl;
		  return;
		}
	  bytesLeft = bytesLeft - bytesRecv;
	}

	  int bytesSent = send(clientSock, (void *) na,
						24, 0);
		if(bytesSent != 24)
			{
			  cout << "bytesSent != 24" << endl;
			  exit(-1);
			}
		  bytesSent = send(clientSock, (void *) nb,
						   24, 0);
		  if(bytesSent != 24)
			{
			  cout << "bytesSent != 24" << endl;
			  exit(-1);
			}
		  bytesSent = send(clientSock, (void *) nc,
						   24, 0);
		  if(bytesSent != 24)
			{
			  cout << "bytesSent != 24" << endl;
			  exit(-1);
			}

}  
    
void *threadMain(void *args)
{
  int clientSock;

  //make sure thread resources are deallocated on return
  pthread_detach(pthread_self());

  //extract the socket FD
  struct ThreadArgs *threadArgs = (struct ThreadArgs *) args;
  clientSock = ((ThreadArgs *) threadArgs)->clientSock;
    
  //communicate with client;
  processClient(clientSock);
  
  //close client socket
  int status = close(clientSock);
  if(status < 0)
	{
	  cout << "Error closing socket" << endl;
	}
  if(status == 0)
	{
	  cout << "Socket closed" << endl;
	}

  delete threadArgs;
  
  return NULL; //done
}

int main(int argc, char* argv[])
{
  //check for proper number of
  //CL arguments

  if (argc < 2)
	{
	  cerr << "Improper number of arguments\n";
	  exit(-1);
	}

  //init the mutext
  pthread_mutex_init(&boardLock, NULL);
  
  //cast the first command line argument
  //as an integer and use it as the server
  //port
  unsigned short servPort =
	atoi(argv[1]);
  
  cout << "Server!" << endl;
  
  cout << "Port passed was " << servPort << endl;
  cout << "Creating Socket..." << endl;

  //Create socket

  int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if( sock < 0)
	{
	  cerr << "Could not create socket." << endl;
	  exit(-1);
	}
  cout << "Socket created, configuring and binding..." << endl;
  //Assign the port to socket and set
  //the fields
  struct sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);

  //bind!
  int status = bind(sock,
					(struct sockaddr *) &servAddr,
				sizeof(servAddr));
  if (status < 0)
	{
	  cerr << "Error with bind, "
		   << "it's likely that the port is in "
		   << "use, try changing the port or "
		   << "waiting" << endl;
	  exit(-1);
	}

  //set the socket to listen
  const int MAXPENDING = 10;
  status = listen(sock, MAXPENDING);
  if(status < 0)
	{
	  cerr << "Unable to set listen" << endl;
	  exit(-1);
	}
  cout << "Socket configured, listening for client..." << endl;

  while(true)
	{
	  //sit and wait until we get a connection
	  //when we do, put it in a sockaddr_in struct
	  //and get int descriptor for it
	  struct sockaddr_in clientAddr;
	  
	  socklen_t addrLen = sizeof(clientAddr);
	  
	  int clientSock = accept(sock,
							  (struct sockaddr *) &clientAddr,
							  &addrLen);
	  if(clientSock < 0)
		{
		  cerr << "Error during accept" << endl;
		  exit(-1);
		}
	  cout << "Client Connected." << endl;

	  //create and init agrument struct
	  ThreadArgs *args = new ThreadArgs;
	  args->clientSock = clientSock;
	  
	  //create client thread
	  pthread_t threadID;
	  int status = pthread_create(&threadID, NULL, threadMain,
								  (void *) args);
	  if(status != 0)
		{
		  cerr << "Problem with thread creation\n";
		  exit(-1);
		}
	}
  
  return 0;
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

//Send a long over the socket
int sendLong(int clientSock, long num)
{
  long networkNum = htonl(num);
	
  int bytesSent = send(clientSock, (void *) &networkNum,
					   sizeof(long), 0);
  if(bytesSent != sizeof(long))
	{
	  return -1;
	}
  return 0;
}

//Receive a long over the socket
int recvLong(int clientSock, long &msg)
{
  int bytesLeft = sizeof(long);
  long networkMsg;
  char* gp = (char *) &networkMsg;

  while(bytesLeft)
	{
	  int bytesRecv = recv(clientSock, gp,
						   bytesLeft, 0);
	  if(bytesRecv <= 0)
		{
		  return -1;
		}
	  bytesLeft = bytesLeft - bytesRecv;
	  gp = gp + bytesRecv;
	}
  msg = ntohl(networkMsg);
  return 0;
}