#include <cstring>
#include <cstdlib>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

bool isValid(int guess)

//receive a long
int recvLong(int sock, long &msg)
{
  int bytesLeft = sizeof(long);
  long networkMsg;
  char *gp = (char *) &networkMsg;

  while(bytesLeft)
	{
	  int bytesRecv = recv(sock, gp, bytesLeft, 0);
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

//send a long
int sendLong(int sock, long num)
{
  long networkNum = htonl(num);
  int bytesSent = send(sock, (void *) &networkNum,
					   sizeof(long), 0);
  if(bytesSent != sizeof(long))
	{
	  return -1;
	}
  return 0;
}

int main(int argc, char* argv[])
{
  if (argc < 3)
	{
	  cerr << "Improper number of arguments\n";
	  cerr << "Proper format: <IP address>, <port #>\n";
	  exit(-1);
	}
  
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

  //play the game

  //get player name
  cout << "Enter your name: ";
  string msgStr;
  char msg[50];
  cin >> msgStr;

  strcpy(msg, msgStr.c_str());
  
  //submit it to the server
  int bytesSent = send(sock, (void *) msg, 24, 0);
  if(bytesSent != 24)
	{
	  cout << "bytesSent != 50" << endl;
	  exit(-1);
	}

  //enter game loop
  bool gameRunning = true;
  while(gameRunning)
	{
	  //get player guess
	  long guess;
	  cout << "enter a guess: ";
	  cin >> guess;

	  if(isValid(guess))
		{
		  //send guess to server
		  status = sendLong(sock, guess);
		  if(status < 0)
			{
			  cout << "Lost connection to server :(" << endl;
			  exit(-1);
			}
		  //receive server answer
		  long hostMsg;
		  status = recvLong(sock, hostMsg);
		  if(status < 0)
			{
			  cout << "Lost connection to server :(" << endl;
			  exit(-1);
			}
		  if(hostMsg == 0)
			{
			  cout << "Correct! Game Over!" << endl;
			  //receive turns
			  long msg;
			  int status = recvLong(sock, msg);
			  if(status < 0)
				{
				  cout << "Lost connection to server :(" << endl;
				  exit(-1);
				}
			  cout << "Number of turns: " << msg << endl;

			  //receive leaderboard info;
			  long score1, score2, score3;

			  //scores
			  status = recvLong(sock, score1);
			  if(status < 0)
				{
				  cout << "Lost connection" << endl;
				  exit(-1);
				}
			  status = recvLong(sock, score2);
			  if(status < 0)
				{
				  cout << "Lost connection" << endl;
				  exit(-1);
				}
			  status = recvLong(sock, score3);
			  if(status < 0)
				{
				  cout << "Lost connection" << endl;
				  exit(-1);
				}

			  //names			  
			  int bytesLeft = 50;
			  char buffera[50];
			  char* ap = buffera;
			  while(bytesLeft > 0)
				{
				  int bytesRecv = recv(sock,
								   &buffera[50-bytesLeft],
								   bytesLeft, 0);
				  if(bytesRecv <= 0)
					{
					  cout << "Lost connection\n";
					  exit(-1);
					}
				  bytesLeft = bytesLeft - bytesRecv;
				}
			  bytesLeft = 50;
			  char bufferb[50];
			  char* bp = bufferb;
			  while(bytesLeft > 0)
				{
				  int bytesRecv = recv(sock,
								   &bufferb[50-bytesLeft],
								   bytesLeft, 0);
				  if(bytesRecv <= 0)
					{
					  cout << "Lost connection\n";
					  exit(-1);
					}
				  bytesLeft = bytesLeft - bytesRecv;
				}

			  bytesLeft = 50;
			  char bufferc[50];
			  char* cp = bufferc;
			  while(bytesLeft > 0)
				{
				  int bytesRecv = recv(sock,
								   &bufferc[50-bytesLeft],
								   bytesLeft, 0);
				  if(bytesRecv <= 0)
					{
					  cout << "Lost connection\n";
					  exit(-1);
					}
				  bytesLeft = bytesLeft - bytesRecv;
				}
			  
			  
			  cout << "Leaderboard:" << endl;

			  if(score1 != 0)
				{
				  cout << "1. " << ap
					   << " " << score1 << endl;
				}
			  if(score2 != 0)
				{
				  cout << "2. " << bp
					   << " " << score2 << endl;
				}
			  if(score3 != 0)
				{
				  cout << "3. " << cp
					   << " " << score3 << endl;
				}

			  
			  gameRunning = false;
			}
		  else
			{
			  cout << "Incorrect. Closeness Value: "
				   << hostMsg << endl;
			}
		}
	  else
		{
		  cout << "Invalid guess" << endl;
		}
	}
  
  return 0;
}
