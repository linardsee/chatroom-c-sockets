#include<iostream>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<errno.h>
#include<string.h>

#define RCV_BUFF_SIZE 512 

using namespace std;

class Csocket
{
	public:
		Csocket(){};
		~Csocket(){};
		void SetTimeout(unsigned int theTimeout);
		int InitServer(char* ip, int port);
		int InitServer(int port);
		int InitClient();
	 	int ConnectClient(char* ip, int port);
		int ConnectClient(int port);
		int Accept();	
		static int SendMessage(int sockfd, char* buff);
		static int ReceiveMessage(int sockfd, char* buff);
	private:
		struct sockaddr_in serv_addr;
		struct sockaddr_in cli_addr;
		struct timeval timeout;
		int socket_fd=0;
};
