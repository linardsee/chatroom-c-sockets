
#include "Csocket.h"


void Csocket::SetTimeout(unsigned int theTimeout)
{
	timeout.tv_sec = theTimeout;
	timeout.tv_usec = 0;
}	

int Csocket::InitServer(char* ip, int port)
{
	int descriptor = socket(AF_INET, SOCK_STREAM, 0);
        int option = 1;
	
	serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);
        serv_addr.sin_port = htons(port);
	
	Csocket::SetTimeout(5);


	if(setsockopt(descriptor, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option, sizeof(option)) < 0)
        {
		std::cerr << "ERROR: setsockopt failed!";
               // perror("ERROR: setsockopt failed");
                return EXIT_FAILURE;
        }
	
	if(setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)) < 0)
        {
		std::cerr << "ERROR: setsockopt failed!";
               // perror("ERROR: setsockopt failed");
                return EXIT_FAILURE;
        }

	if(setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO,(char*)&timeout, sizeof(timeout)) < 0)
        {
		std::cerr << "ERROR: setsockopt failed!";
               // perror("ERROR: setsockopt failed");
                return EXIT_FAILURE;
        }


        /* Bind */
        if(bind(descriptor, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        {
		std::cerr << "ERROR: Socket binding failed!";
               // perror("ERROR: Socket binding failed");
                return EXIT_FAILURE;
        }

        /* Listen */
        if (listen(descriptor, 10) < 0)
        {
		std::cerr << "ERROR: Socket listening failed!";
                // perror("ERROR: Socket listening failed");
                return EXIT_FAILURE;
        }

	socket_fd = descriptor;

	return descriptor;
}

//Overloading for loopback address case
int Csocket::InitServer(int port)
{
	char ip[] = "127.0.0.1";
	int descriptor = socket(AF_INET, SOCK_STREAM, 0);
        int option = 1;
	
	serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);
        serv_addr.sin_port = htons(port);

	Csocket::SetTimeout(5);

	if(setsockopt(descriptor, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
        {
		std::cerr << "ERROR: setsockopt failed!";
               // perror("ERROR: setsockopt failed");
                return EXIT_FAILURE;
        }

	if(setsockopt(descriptor, SOL_SOCKET, SO_RCVTIMEO,(char*)&timeout, sizeof(timeout)) < 0)
        {
		std::cerr << "ERROR: setsockopt failed!";
               // perror("ERROR: setsockopt failed");
                return EXIT_FAILURE;
        }

	if(setsockopt(descriptor, SOL_SOCKET, SO_SNDTIMEO,(char*)&timeout, sizeof(timeout)) < 0)
        {
		std::cerr << "ERROR: setsockopt failed!";
               // perror("ERROR: setsockopt failed");
                return EXIT_FAILURE;
        }


        /* Bind */
        if(bind(descriptor, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        {
		std::cerr << "ERROR: Socket binding failed!";
               // perror("ERROR: Socket binding failed");
                return EXIT_FAILURE;
        }

        /* Listen */
        if (listen(descriptor, 10) < 0)
        {
		std::cerr << "ERROR: Socket listening failed!";
                // perror("ERROR: Socket listening failed");
                return EXIT_FAILURE;
        }
	
	socket_fd = descriptor;

	return descriptor;
}

int Csocket::InitClient()
{
	int descriptor = socket(AF_INET, SOCK_STREAM, 0);		      socket_fd = descriptor;

	return descriptor;
}

int Csocket::ConnectClient(char* ip, int port)
{
	serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);
        serv_addr.sin_port = htons(port);

	int err = connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	return err;
}

//Overloading for loopback address case
int Csocket::ConnectClient(int port)
{
	char ip[] = "127.0.0.1";

	serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = inet_addr(ip);
        serv_addr.sin_port = htons(port);

	int err = connect(socket_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	return err;
}

int Csocket::Accept()
{
	socklen_t clilen = sizeof(cli_addr);
	int connfd = accept(socket_fd, (struct sockaddr*)&cli_addr, &clilen);

	return connfd;
}

int Csocket::SendMessage(int sockfd, char* buff)
{
	char ack;
	int msgSent = 0;
	uint8_t len[2] = {};
	uint16_t length = strlen(buff);
	unsigned int msgLeft = length;

	len[0] = (length & 0xFF00) >> 8;
	len[1] = length & 0xFF;

	while(msgSent != 2)
	{
		msgSent = send(sockfd, len, sizeof(len), 0);
		if(msgSent == -1)
			return -1;
	}

	if(recv(sockfd, &ack, 1, MSG_WAITALL) == -1)
	{
		cout << "Error: receiving ack\n";
		return -1;
	}

	msgSent = 0;

	while(msgLeft > 0)
	{
		msgSent = send(sockfd, buff + length - msgLeft, strlen(buff + length - msgLeft), 0);
		msgLeft -= msgSent;
		if(msgSent == -1)
			return -1;
	}

	if(recv(sockfd, &ack, 1, MSG_WAITALL) == -1)
	{
		cout << "Error: receiving ack\n";
		return -1;
	}

	return 1;
}

int Csocket::ReceiveMessage(int sockfd, char* buff)
{
	int msgRcvd = 0;
	int sent_ack = 0;
	unsigned int msgLeft = 0;
	uint8_t len[2] = {};
	uint16_t length = 0;
	char tempBuff[RCV_BUFF_SIZE] = {};
	char ack;

	msgRcvd = recv(sockfd, len, sizeof(len), MSG_WAITALL); // blocking function, returns only when all bytes have been received
	if(msgRcvd == -1)
		return -1;
	if(msgRcvd == 0)
		return 0;
	
	while(!sent_ack)
	{
		sent_ack = send(sockfd, &ack, 1, 0);
		if(sent_ack == -1)
		{
			cout << "Error: unable to send ack\n";
			return -1;
		}
	}
	
	sent_ack = 0;

	length |= (len[0] << 8) | len[1];
	
	msgRcvd = 0;
	msgLeft = length;
	
	while(msgLeft > 0)
	{
		msgRcvd = recv(sockfd, tempBuff, sizeof(tempBuff), 0);
		if(msgRcvd == -1)
			return -1;
		if(msgRcvd == 0)
			return 0;
		msgLeft -= msgRcvd;
		strcat(buff, tempBuff);
		bzero(tempBuff, RCV_BUFF_SIZE);
	}
	
	while(!sent_ack)
	{
		sent_ack = send(sockfd, &ack, 1, 0);
		if(sent_ack == -1)
		{
			cout << "Error: unable to send ack\n";
			return -1;
		}
	}


	return 1;
}
