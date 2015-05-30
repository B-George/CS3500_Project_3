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



struct leaderboard
{
  string name1;
  int score1;
  string name2;
  int score2;
  string name3;
  int score3;
};

//globals
leaderboard board;
pthread_mutex_t boardLock;

struct ThreadArgs
{
  int clientSock;
};

void printLeaderboard()
{
  cout << "Leaderboard: " << endl;

  if(board.score1 != 0)
	{
	  cout << "1. " << board.name1 << ", " << board.score1 << endl;
	}
  if(board.score2 != 0)
	{
	  cout << "2. " << board.name2 << ", " << board.score2 << endl;
	}
  if(board.score3 != 0)
	{
	  cout << "3. " << board.name3 << ", " << board.score3 << endl;
	}
}

//Updates the leaderboard
void updateLeaderboard(char* name, int score)
{//remember that a lower score is better



  //stick score in the first available slot, or the first slot
  //whose score it beats
  
  if(score < board.score1 || board.score1 == 0)
	{
	  pthread_mutex_lock(&boardLock);
	  
	  //move the names down

	  board.score3 = board.score2;
	  board.name3 = board.name2;


	  board.score2 = board.score1;
	  board.name2 = board.name1;
	  board.score1 = score;
	  board.name1 = name;
	  pthread_mutex_unlock(&boardLock);
	}
  else if(score < board.score2 || board.score2 == 0)
	{
	  pthread_mutex_lock(&boardLock);
	  
	  board.name3 = board.name2;
	  board.score3 = board.score2;

	  board.name2 = name;
	  board.score2 = score;
	  pthread_mutex_unlock(&boardLock);
	}
  else if(score < board.score3 || board.score3 == 0)
	{
	  pthread_mutex_lock(&boardLock);
	  
	  board.name3 = name;
	  board.score3 = score;
	  pthread_mutex_unlock(&boardLock);
	}

  printLeaderboard();
}
  
//Returns a new random number, the one the
//player is trying to guess
int newRandom()
{
  srand(time(NULL));
  int newRand = rand() % 10000;
  return newRand;
}

//Returns the integer representation of
//how close the player was
int getCloseness(int guess, int answer)
{
  int ans_a = answer / 1000;
  int ans_b = answer % 1000;
  ans_b = ans_b / 100;
  int ans_c = answer % 100;
  ans_c = ans_c / 10;
  int ans_d = answer % 10;

  cout << ans_a << endl
	   << ans_b << endl
	   << ans_c << endl
	   << ans_d << endl;

  cout << endl;

  int guess_a = guess / 1000;
  int guess_b = guess % 1000;
  guess_b = guess_b / 100;
  int guess_c = guess % 100;
  guess_c = guess_c / 10;
  int guess_d = guess % 10;

  cout << guess_a << endl
	   << guess_b << endl
	   << guess_c << endl
	   << guess_d << endl;

  int a = abs(ans_a - guess_a);
  int b = abs(ans_b - guess_b);
  int c = abs(ans_c - guess_c);
  int d = abs(ans_d - guess_d);
  int cVal = a + b + c + d;
  return cVal;  
}

//Check if the answer is correct
bool isCorrect(int guess, int answer)
{
  return (guess == answer);
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

//The main client-server interaction loop
void processClient(int clientSock)
{
  //get client name

  cout << "Receiving player name..." << endl;
  
  int bytesLeft = 50;
  char buffer[50];
  char* bp = buffer;
  while(bytesLeft > 0)
	{
	  int bytesRecv = recv(clientSock, &buffer[50-bytesLeft],
						   bytesLeft, 0);
	  if(bytesRecv <= 0)
		{
		  cout << "Probable client disconnect" << endl;
		  return;
		}
	  bytesLeft = bytesLeft - bytesRecv;
	}
  cout << bp << endl; //do something else here !**

  //enter guessing loop

  bool gameRunning = true;
  int answer = newRandom();
  int turns = 1;
  while(gameRunning)
	{

	  //receive player guess
	  long guess;
	  int status = recvLong(clientSock, guess);
	  if(status < 0)
		{
		  cout << "Probable client disconnect" << endl;
		  return;
		}
	  
	  cout << "Player guess: " << guess << endl;
	  
	  //send an answer
	  if( isCorrect(guess, answer))
		{
		  updateLeaderboard(bp, turns);
		  //send 0
		  status = sendLong(clientSock, 0);
		  if(status < 0)
			{
			  cout << "Probable client disconnect" << endl;
			  return;
			}

		  //send number of turns;
		  status = sendLong(clientSock, turns);
		  if(status < 0)
			{
			  cout << "Probable client disconnect" << endl;
			  return;
			}

		  //send leaderBoard information
		  status = sendLong(clientSock, board.score1);
		  if(status < 0)
			{
			  cout << "Probable client disconnect"
				   << endl;
			}
		  status = sendLong(clientSock, board.score2);
		  if(status < 0)
			{
			  cout << "Probable client disconnect"
				   << endl;
			}
		  status = sendLong(clientSock, board.score3);
		  if(status < 0)
			{
			  cout << "Probable client disconnect"
				   << endl;
			}

		  char na[50];
		  char nb[50];
		  char nc[50];

		  strcpy(na, board.name1.c_str());
		  strcpy(nb, board.name2.c_str());
		  strcpy(nc, board.name3.c_str());

		  int bytesSent = send(clientSock, (void *) na,
						   50, 0);
		  if(bytesSent != 50)
			{
			  cout << "bytesSent != 50" << endl;
			  exit(-1);
			}
		  bytesSent = send(clientSock, (void *) nb,
						   50, 0);
		  if(bytesSent != 50)
			{
			  cout << "bytesSent != 50" << endl;
			  exit(-1);
			}
		  bytesSent = send(clientSock, (void *) nc,
						   50, 0);
		  if(bytesSent != 50)
			{
			  cout << "bytesSent != 50" << endl;
			  exit(-1);
			}
		  
		  //end game
		  gameRunning = false;
		}
	  else
		{
		  //send closeness value
		  int cVal = getCloseness(guess, answer);
		  cout << "cVal: " << cVal << endl;
		  sendLong(clientSock, cVal);
		  turns++;
		}
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
  cout << "Creating leaderboard..." << endl;
  board.name1 = "None";
  board.score1 = 0;
  board.name2 = "None";
  board.score2 = 0;
  board.name3 = "None";
  board.score3 = 0;
  
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
  const int MAXPENDING = 5;
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
