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

struct vote_msg {
	unsigned short magic;
	unsigned char flag, type;
	unsigned long req_ID, check_sum, candidate, vote_count, cookie;
};


// globals

vote_msg candidate_info[65536];

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
void processClient(int clientSock);  
    
// thread processing
void *threadMain(void *args);

int hashCand(unsigned long candNum);

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
	out_buffer[3] = (cookie >> 24) & 0xFF;
	out_buffer[2] = (cookie >> 16) & 0xFF;
	out_buffer[1] = (cookie >> 8) & 0xFF;
	out_buffer[0] = (cookie) & 0xFF;
	out_buffer[7] = (vote_count >> 24) & 0xFF;
	out_buffer[6] = (vote_count >> 16) & 0xFF;
	out_buffer[5] = (vote_count >> 8) & 0xFF;
	out_buffer[4] = (vote_count) & 0xFF;
	out_buffer[11] = (candidate >> 24) & 0xFF;
	out_buffer[10] = (candidate >> 16) & 0xFF;
	out_buffer[9] = (candidate >> 8) & 0xFF;
	out_buffer[8] = (candidate) & 0xFF;
	out_buffer[15] = (check_sum >> 24) & 0xFF;
	out_buffer[14] = (check_sum >> 16) & 0xFF;
	out_buffer[13] = (check_sum >> 8) & 0xFF;
	out_buffer[12] = (check_sum) & 0xFF;
	out_buffer[19] = (req_ID >> 24) & 0xFF;
	out_buffer[18] = (req_ID >> 16) & 0xFF;
	out_buffer[17] = (req_ID >> 8) & 0xFF;
	out_buffer[16] = (req_ID) & 0xFF;	
	out_buffer[23] = (first_row >> 24) & 0xFF;
	out_buffer[22] = (first_row >> 16) & 0xFF;
	out_buffer[21] = (first_row >> 8) & 0xFF;
	out_buffer[20] = (first_row) & 0xFF;
}

unsigned long getCheckSum(unsigned long tmpRe, unsigned long tmpCandi,
							unsigned long tmpVC, unsigned long tmpCk)
{
	return CSUM ^ tmpRe ^ tmpCandi ^ tmpVC ^ tmpCk;
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

//The main client-server interaction loop
void processClient(int clientSock)
{
	bool isGood = false;
	unsigned short tmpMag;
	unsigned char tmpFlag, tmpType;
  unsigned long tmpReq, tmpCSum, tmpCand, tmpVot, tmpCook;
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
 //**** process buffer 
	(tmpMag >> 8) & 0xFF = buffer[23];
	(tmpMag) & 0xFF = buffer[22];
	(tmpFlag) & 0xFF = buffer[21];
	(tmpType) & 0xFF = buffer[20];
	(tmpReq >> 24) & 0xFF = buffer[19];
	(tmpReq >> 16) & 0xFF = buffer[18];
	(tmpReq >> 8) & 0xFF = buffer[17];
	(tmpReq) & 0xFF = buffer[16];
	(tmpCSum >> 24) & 0xFF = buffer[15];
	(tmpCSum >> 16) & 0xFF = buffer[14];
	(tmpCSum >> 8) & 0xFF = buffer[13];
	(tmpCSum) & 0xFF = buffer[12];
	(tmpCand >> 24) & 0xFF = buffer[11];
	(tmpCand >> 16) & 0xFF = buffer[10];
	(tmpCand >> 8) & 0xFF = buffer[9];
	(tmpCand) & 0xFF = buffer[8];
	(tmpVot >> 24) & 0xFF = buffer[7];
	(tmpVot >> 16) & 0xFF = buffer[6];
	(tmpVot >> 8) & 0xFF = buffer[5];
	(tmpVot) & 0xFF = buffer[4];
	(tmpCook >> 24) & 0xFF = buffer[3];
	(tmpCook >> 16) & 0xFF = buffer[2];
	(tmpCook >> 8) & 0xFF = buffer[1];
	(tmpCook) & 0xFF = buffer[0];
	ntohs(tmpMag);
	
	if(tmpMag != MAGIC){
		cout << "Error. Magic is not.\n\n";
		
		// do shit with bad mag
		tmpFlag |= 1 << 2;
	}
	if((tmpFlag >> 6) & 1){
		tmpFlag |= 1 << 3;
	}
	// process flags
	if(!((tmpFlag >> 7) & 1)){
		isGood = true;
		cout << "No check_sum for this msg. Bad dog.\n\n";
		// do shit for no checksum
		// process request
	}else if(getCheckSum(tmpReq, tmpCand, tmpVot, tmpCook) != CSUM){
			cout << "incorrect checksum. bad dog.\n\n";
			tmpFlag |= 1 << 1;
			isGood = false;
			//process error
	}else{
		isGood = true;
	}	
	if(isGood){
		ntohl(tmpReq);
		ntohl(tmpCand); 
		ntohl(tmpVot);
		ntohl(tmpCook);
		
		// do shit for good checksum
		// check type
		
		// if inquiry do inquiry shit
		if(tmpType == TYPEINQ) {
			processInquiry(tmpCand);
		}else if(tmpType == TYPEVOTE){
			processVote(tmpCand);
		// else record vote
		}else{
			tmpFlag |= 1 << 0;
		}
	}			
	// send reply shit
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

int hashCand(unsigned long candNum)
{
	return candNum %(65536);
}