#include <cstring>
#include <cstdlib>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <stdio.h>

using namespace std;



//globals

pthread_mutex_t boardLock;

struct ThreadArgs
{
  int clientSock;
};

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

//The main client-server interaction loop
void processClient(int clientSock)
{
  //get client name

  cout << "Receiving player name..." << endl;
  
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
