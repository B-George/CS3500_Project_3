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
#include <signal.h>

#define REP(a,b) for(int i = a; a < b, a++);
#define TYPEVOTE 0x18
#define TYPEINQ 0x08
#define VOTECOUNT 0
#define CSUM 0xdeadbeef
#define MAGIC 0x023b
#define PBIT 0
#define MBIT 0 
#define SBIT 0 
#define TBIT 0 
#define DEBUG 1
#define struck struct

using namespace std;

struck vote_msg {
	unsigned long ReqID, Candidate, VoteCount, Cookie;
};

// constants
const int LENGTH = 24;

// globals

vote_msg candidate_info[65536];

int sighandle = 1;
bool cbit = false; // checksum flag
bool request = false; // bit 6 of flag field, 1 = response, 0 = request
unsigned long first_row, req_ID, check_sum, candidate, vote_count, cookie;
unsigned short magic = MAGIC;
char flags, type;
unsigned char out_buffer[24];
unsigned char in_buffer[24];
unsigned int numMsg = 0;
unsigned int numVotes = 0;
unsigned int numInq = 0;
unsigned int num_f_u_ = 0;
int sock;

pthread_mutex_t boardLock;

struck ThreadArgs
{
  int clientSock;
};

// sets first row of data(magic, flags, message type)
void setFirstRow(unsigned short mag, char flag, char msg_type); 

// place bytes in buffer
void fillBuff();

// calculate checksum
unsigned long getCheckSum(unsigned long tmpRe, unsigned long tmpCandi,
							unsigned long tmpVC, unsigned long tmpCk);

// send all bits in buffer
// socket == socket to send data to
// buf == data buffer
// *len == pointer to buffer length in bytes (int)
int sendall(int socket);

//Send a long over the socket
int sendLong(int clientSock, long num);

//Receive a long over the socket
int recvLong(int clientSock, long &msg);

//The main client-server interaction loop
void processClient(int clientSock);  
    
// thread processing
void *threadMain(void *args);

// calculate hash Number for array indexing
int hashCand(unsigned long candNum);

// send inquiry reply
void processInquiry(unsigned long r, unsigned long c, unsigned char f);

// process votes and send reply
void processVote(unsigned long r, unsigned long c, 
				unsigned long vc, unsigned long omnomnomnomnom);

// signal handler
void handler(int s);

// display statistics
void printStats();

// craft and send reply
void sendReply(int clientSock);

int main(int argc, char* argv[])
{
	signal(SIGINT, handler);
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
  cout << "Creating Socket...\n\n";

  //Create socket

  sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if( sock < 0)
	{
	  cerr << "Could not create socket.\n\n";
	  exit(-1);
	}
  cout << "Socket created, configuring and binding...\n\n";
  //Assign the port to socket and set
  //the fields
  struck sockaddr_in servAddr;
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(servPort);

  //bind!
  int status = bind(sock,
					(struck sockaddr *) &servAddr,
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
  cout << "Socket configured, listening for client...\n\n" << endl;

  cout << "Type ctrl-c to exit\n\n\n"
  while(sighandle == 1)
	{
	  //sit and wait until we get a connection
	  //when we do, put it in a sockaddr_in struck
	  //and get int descriptor for it
	  struck sockaddr_in clientAddr;
	  
	  socklen_t addrLen = sizeof(clientAddr);
	  
	  int clientSock = accept(sock,
							  (struck sockaddr *) &clientAddr,
							  &addrLen);
	  if(clientSock < 0)
		{
		  cerr << "Error during accept" << endl;
		  exit(-1);
		}
	  cout << "Client Connected." << endl;

	  //create and init agrument struck
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
  printStats();
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

void fillBuff()
{
	// set rows and convert to network byte order
	first_row = htonl(first_row);
	req_ID = htonl(req_ID);
	candidate = htonl(candidate);
	vote_count = htonl(vote_count);
	cookie = htonl(cookie);
	check_sum = getCheckSum(req_ID, candidate, vote_count, cookie);
	
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

int sendall()
{
		int total = 0;
		int bytesleft = LENGTH;
		int n;
		
		while(total < LENGTH) {
			n = send(socket, out_buffer+total, bytesleft, 0);
			if (n == -1) {break; }
			total += n;
			bytesleft -= n;
		}

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
	numMsg++;
 //**** process buffer 
	tmpMag = (buffer[23] << 8) + (buffer[22]);
	tmpFlag = buffer[21];
	tmpType = buffer[20];
	tmpReq = (buffer[19] << 24) + (buffer[18] << 16) + (buffer[17] << 8) + (buffer[16]);
	tmpCSum = (buffer[15] << 24) + (buffer[14] << 16) + (buffer[13] << 8) + (buffer[12]);
	tmpCand = (buffer[11] << 24) + (buffer[10] << 16) + (buffer[9] << 8) + (buffer[8]);
	tmpVot = (buffer[7] << 24) + (buffer[6] << 16) + (buffer[5] << 8) + (buffer[4]);
	tmpCook = (buffer[3] << 24) + (buffer[2] << 16) + (buffer[1] << 8) + (buffer[0]);
	
	#if DEBUG == 1
		cout << "\nmessage contents\nmagic = " << tmpMag << endl;
		cout << "Flag = " << tmpFlag << endl;
		cout << "Type = " << tmpType << endl;
		cout << "Req_ID = " << tmpReq << endl;
		cout << "check_sum = " << tmpCSum << endl;
		cout << "candidate = " << tmpCand << endl;
		cout << "vote ct = " << tmpVot << endl;
		cout << "cookie = " << tmpCook << endl << endl;
	#endif
	
	ntohs(tmpMag);
	
	if(tmpMag != MAGIC){
		cout << "Error. Magic is not.\n\n";
		num_f_u_++;
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
			num_f_u_++;
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
			pthread_mutex_lock(&boardLock);
			processInquiry(tmpReq, tmpCand, tmpFlag);
			sendall();
			pthread_mutex_unlock(&boardLock);
		}else if(tmpType == TYPEVOTE){
		
			pthread_mutex_lock(&boardLock);
			processVote(tmpReq, tmpCand, tmpVot, tmpCook, tmpFlag);
			sendall();
			pthread_mutex_unlock(&boardLock);
			
		}else{
			num_f_u_++
			tmpFlag |= 1 << 0;
		}
	}			

}  
    
void *threadMain(void *args)
{
  int clientSock;

  //make sure thread resources are deallocated on return
  pthread_detach(pthread_self());

  //extract the socket FD
  struck ThreadArgs *threadArgs = (struck ThreadArgs *) args;
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

void processInquiry(unsigned long r, unsigned long c, unsigned char f)
{
	int hashNo = hashCand(c);
	setFirstRow(MAGIC, f, TYPEINQ);
	req_ID = r;
	candidate = c;
	vote_count = candidate_info[hashNo].VoteCount;
	cookie = rand();
	fillBuff();
}

void processVote(unsigned long r, unsigned long c, 
					unsigned long vc, unsigned long omnomnomnomnom, unsigned char f)
{
	int hashNo = hashCand(c);
	candidate_info[hashNo].ReqID = r;
	candidate_info[hashNo].Candidate = c;
	candidate_info[hashNo].VoteCount = vc;
	candidate_info[hashNo].Cookie = omnomnomnomnom;
	setFirstRow(MAGIC, f, TYPEVOTE)
	fillBuff();
}

void handler(int s)
{
	sighandle = 0;
}

void printStats()
{
	cout << "voting history: \n\n";
	REP(0, 65536) {
		if ( candidate_info[i].req_ID != 0 ){
			cout << "\ncandidate: " << candidate_info[i].Candidate << endl;
			cout << "votes: " << candidate_info[i].VoteCount << endl;
	}
}

