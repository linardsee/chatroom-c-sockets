
#include "Csocket.h"
#include "CClient.h"
#include "CRoom.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <list>
#include <cstdlib>
#include <algorithm>


#define MAX_EPOLL_EVENTS 64
#define MAX_BUFF_SIZE 1024

/* GLOBAL VARIABLES */
list<CClient*> OnlineClients;
list<CClient> OfflineClients;
list<CClient*>::iterator it_on;
list<CClient>::iterator it_off;
list<CRoom> Rooms;
list<CRoom>::iterator it_room;
CRoom room;
/* GLOBAL VARIABLES ENDS */

using namespace std;

/* FUNCTORS */
class FindByName
{
        string name;

        public:
                FindByName(const string& name) : name(name) {}
			
                bool operator()( CRoom room) const
                {
                        return !room.getRoomName().compare(name);
                }
};

class FindByNameCl
{
        string name;

        public:
                FindByNameCl(const string& name) : name(name) {}

                bool operator()(CClient* client) const
                {
                        return !name.compare(client->getClientName());
                }
};

class FindByNameClient
{
        string name;

        public:
                FindByNameClient(const string& name) : name(name) {}

                bool operator()(CClient client) const
                {
                        return !name.compare(client.getClientName());
                }
};


class FindBySockfd
{
        int sockfd;

        public:
                FindBySockfd(const int& sockfd) : sockfd(sockfd) {}

                bool operator ()(CClient* client) const
                {
                        return client->getSockfd() == sockfd;
                }
};
/* FUNCTORS ENDS */

int handleCreateCommand(int sockfd)
{
        char buff[MAX_BUFF_SIZE] = {'\0'};
        string buffStr;

        if (Csocket::ReceiveMessage(sockfd, buff) == -1)
        {
                cout << "Error: receiving  message in handleCreateCommand\n";
                return -1;
        }
        buffStr = buff;
        bzero(buff, MAX_BUFF_SIZE);
	
        it_room = find_if(Rooms.begin(), Rooms.end(), FindByName(buffStr));


        if(it_room == Rooms.end())
        {
	        Rooms.push_back(room);
                it_room = --Rooms.end();
		it_room->setRoomName(buffStr);
			
		it_on = OnlineClients.begin();


       		it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindBySockfd(sockfd));
	
                if( ((*it_on)->getLastRoom()) != "")
                {
                        (*it_on)->setLastRoom(buffStr);
                        ++(*it_room);

                        it_room = find_if(Rooms.begin(), Rooms.end(), FindByName((*it_on)->getLastRoom()));
                        --(*it_room);
                }
                else
                {
                        (*it_on)->setLastRoom(buffStr);
                        ++(*it_room);
                }

                stpcpy(buff, "OK");
		Csocket::SendMessage(sockfd, buff);
                cout << (*it_on)->getClientName() << " created " << buffStr << endl;
		
                return 1;
        }
        else
        {
                stpcpy(buff, "ERROR");
		Csocket::SendMessage(sockfd, buff);
                cout << "Invalid room name\n";

                return 2;
        }

}

int handleClientName(CClient* theClient)
{
	char theName[32] = {};
	char sendBuff[MAX_BUFF_SIZE] = {};
	string nameStr;
	int state = 0;
	int receive = 0;
	
	// Get the name string
	while(receive == 0)
	{
		receive = Csocket::ReceiveMessage(theClient->getSockfd(), theName);
		if( receive == -1 )
		{
			cout << "Error: receiving client's name\n";
			return -1;
		}	
	}
	nameStr = theName;
	
	// Check the contents of name string
	if( (nameStr.length() < 2) || (nameStr.length() >= 31) )
	{
		cout << "Login size is not valid\n";
		strcpy(sendBuff, "ERROR");
		if( Csocket::SendMessage(theClient->getSockfd(), sendBuff) == -1 )
		{
			cout << "Error: sending error reply\n";
			return -1;
		}
		close(theClient->getSockfd());
		delete theClient;
		bzero(sendBuff, MAX_BUFF_SIZE);
		return 0;
	}
	else
	{
		// First check online list to avoid collision on same login names
		for(it_on = OnlineClients.begin(); it_on != OnlineClients.end(); ++it_on)
		{
			if( (*it_on)->getClientName().compare(nameStr) == 0 )
			{
				strcpy(sendBuff, "ERROR");
				if( Csocket::SendMessage(theClient->getSockfd(), sendBuff) == -1 )
				{
					cout << "Error: sending error reply\n";
					return -1;
				}
				close(theClient->getSockfd());
				delete theClient;
				bzero(sendBuff, MAX_BUFF_SIZE);
				return 0;
			}
		}
	
		// Now check the offline list to check if login name is already registered 	
		for(it_off = OfflineClients.begin(); it_off != OfflineClients.end(); ++it_off)
		{
			if( (*it_off).getClientName().compare(nameStr) == 0 )
			{
				theClient->setClientName(nameStr);
				theClient->setLastRoom((*it_off).getLastRoom());
				OnlineClients.push_back(theClient);
				
				strcpy(sendBuff, (*it_off).getLastRoom().c_str());

				if( Csocket::SendMessage(theClient->getSockfd(), sendBuff) == -1 ) 
				{
					cout << "Error: sending error reply\n";
					return -1;
				}
				
				cout << nameStr << " connected\n";	
				return 1;	
			}
		}
		
		// If client is not present in both online and offline, then make new
		theClient->setClientName(nameStr);

		OnlineClients.push_back(theClient);
		OfflineClients.push_back(*theClient);
		cout << nameStr << " connected\n";

		strcpy(sendBuff, "OK");
		if( Csocket::SendMessage(theClient->getSockfd(), sendBuff) == -1 ) 
		{
			cout << "Error: sending error reply\n";
			return -1;
		}
	}

	return 1;
}

int broadcastMsg(int sockfd, char* buff, string room)
{
        char sendBuff[MAX_BUFF_SIZE + 32] = {};
	string sender;

	it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindBySockfd(sockfd));
	sender = (*it_on)->getClientName();

        for(it_on = OnlineClients.begin(); it_on != OnlineClients.end(); ++it_on)
        {
		if((*it_on)->getSockfd() == sockfd)
		{
			// Dont broadcast to sender
			continue;
		}
                if( room.compare( (*it_on)->getLastRoom() ) == 0)
                {
			sprintf(sendBuff, "%s@%s>%s", sender.c_str(), room.c_str(), buff);
			if( Csocket::SendMessage((*it_on)->getSockfd(), sendBuff) == -1)
			{
				cout << "Error broadcasting message\n";
				return -1;
			}
                }
        }

        return 0;
}

int handleJoinCommand(int sockfd)
{
        char buff[MAX_BUFF_SIZE] = {'\0'};
        string buffStr;

        if (Csocket::ReceiveMessage(sockfd, buff) == -1)
        {
                        cout << "Error: receiving  message in handleJoinCommand\n";
                        return -1;
        }
        buffStr = buff;
        bzero(buff, MAX_BUFF_SIZE);

        it_room = find_if(Rooms.begin(), Rooms.end(), FindByName(buffStr));

        if(it_room != Rooms.end())
        {
                ++(*it_room);
                it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindBySockfd(sockfd));

                it_room = find_if(Rooms.begin(), Rooms.end(), FindByName((*it_on)->getLastRoom()));
                --(*it_room);

                (*it_on)->setLastRoom(buffStr);
                stpcpy(buff, "OK");
                Csocket::SendMessage(sockfd, buff);
                cout << (*it_on)->getClientName() << " joined " << buffStr << endl;
        }
        else
        {
                stpcpy(buff, "ERROR");
                Csocket::SendMessage(sockfd, buff);
                cout << "Invalid room\n";

                return 2;
        }
	return 0;
}

int handleShowCommand(int sockfd)
{
        char len = (char)Rooms.size();
	char buff[MAX_BUFF_SIZE] = {};

        if (Csocket::SendMessage(sockfd, &len) == -1)
        {
                cout << "Error: sending length in handleShowCommand\n";
                return -1;
        }

	for(it_room = Rooms.begin(); it_room != Rooms.end(); ++it_room)
	{
		stpcpy(buff, (it_room->getRoomName()).c_str());
                if (Csocket::SendMessage(sockfd, buff) == -1)
                {
                        cout << "Error: sending data in handleShowCommand\n";
                        return -1;
                }
                bzero(buff, MAX_BUFF_SIZE);

	}
        cout << "List of rooms sent\n";

        return 0;
}

int handleTextMsg(int sockfd, string str)
{
        string roomName;
	char bdcastMsg[MAX_BUFF_SIZE] = {}; 
	strcpy(bdcastMsg, str.c_str());

        it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindBySockfd(sockfd));
        roomName = (*it_on)->getLastRoom();
        broadcastMsg(sockfd, bdcastMsg, roomName);

        return 0;
}

int printAllRooms()
{
        cout << "Printing rooms...\n";

        for(it_room = Rooms.begin(); it_room != Rooms.end(); ++it_room)
        {
                        cout << it_room->getRoomName() << endl;
        }

        return 0;
}

int printOnlineMembers()
{
        cout << "Printing online members...\n";

        for(it_on = OnlineClients.begin(); it_on != OnlineClients.end(); ++it_on)
        {
                cout << (*it_on)->getClientName() << endl;
        }

        return 0;
}

int printOfflineMembers()
{
        cout << "Printing offline members...\n";

        for(it_off = OfflineClients.begin(); it_off != OfflineClients.end(); ++it_off)
        {
                cout << it_off->getClientName() << endl;
        }

        return 0;
}

int kickMember()
{
        char buff[128] = "KICK";
        string name;

        cout << "Enter member name to kick: ";
        getline(cin, name);

        it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindByNameCl(name));

        if(it_on != OnlineClients.end())
        {
		if( ((*it_on)->getLastRoom()).size() == 0 )
		{
			cout << "Client is not in room\n"; 
		}
		else
		{
			Csocket::SendMessage((*it_on)->getSockfd(), buff);

			sprintf(buff, "%s kicked", ((*it_on)->getClientName()).c_str());	
			(*it_on)->setLastRoom("");
			broadcastMsg((*it_on)->getSockfd(), buff, (*it_on)->getLastRoom());
        	}
	}
        else
        {
                cout << "Error: name not found\n";
        }

        return 0;
}

int removeMember()
{
	char buff[128] = "REMOVE";
        string name, lastRoom;
	int sockfd;

        cout << "Enter member name to remove: ";
        getline(cin, name);

        it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindByNameCl(name));
	it_off = find_if(OfflineClients.begin(), OfflineClients.end(), FindByNameClient(name));	
	sockfd = (*it_on)->getSockfd();
	lastRoom = (*it_on)->getLastRoom();

	if(it_off != OfflineClients.end())
	{
		if(it_on != OnlineClients.end())
		{
						
			Csocket::SendMessage(sockfd, buff);

			sprintf(buff, "%s removed", name.c_str());	
			broadcastMsg(sockfd, buff, lastRoom);
			cout << name << " removed\n";
			it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindByNameCl(name));
			it_off = find_if(OfflineClients.begin(), OfflineClients.end(), FindByNameClient(name));	
		
			close(sockfd);
			OnlineClients.erase(it_on);
		}
		OfflineClients.erase(it_off);
		delete *it_on;
	}
	else
	{
		cout << "Error: no such client\n";
	}	
	
	return 0;
}

int printCommands()
{
	cout << "Commands:\n";
	cout << "PRINT ALL ROOMS -> Show available rooms\n";
	cout << "PRINT ONLINE MEMBERS -> Show all the members online\n"; 
	cout << "PRINT OFFLINE MEMBERS -> Show all the members offline\n"; 
	cout << "KICK -> Kick a member from its current room\n"; 
	cout << "REMOVE -> Remove a member from server\n";
	cout << "PRINT COMMANDS -> Show this menu\n";
	cout << "EXIT -> exit server\n";

	return 0;
}

int handleInput(string rcvStr)
{
        if( rcvStr.compare("PRINT ALL ROOMS") == 0 )
        {
                printAllRooms();
        }
        else if( rcvStr.compare("PRINT ONLINE MEMBERS") == 0 )
        {
                printOnlineMembers();
        }
        else if( rcvStr.compare("PRINT OFFLINE MEMBERS") == 0 )
        {
                printOfflineMembers();
        }
        else if( rcvStr.compare("KICK") == 0 )
        {
                kickMember();
        }
	else if( rcvStr.compare("REMOVE") == 0)
	{
		removeMember();
	}
	else if( rcvStr.compare("PRINT COMMANDS") == 0 )
	{
		printCommands();
	}
	else if( rcvStr.compare("EXIT") == 0 )
        {
                cout << "Bye!\n";
                exit(EXIT_SUCCESS);
        }
        else
        {
        	cout << "Command not found\n";
        }
	
        return 0;
}

int handleReceiveMsg(string rcvStr, int sockfd)
{
        if( rcvStr.compare("JOIN") == 0 )
        {
                handleJoinCommand(sockfd);
        }
        else if( rcvStr.compare("CREATE") == 0 )
        {
                handleCreateCommand(sockfd);
        }
        else if( rcvStr.compare("SHOW") == 0 )
        {
                handleShowCommand(sockfd);
        }
	else
	{
		handleTextMsg(sockfd, rcvStr);
	}

	return 0;
}

int main(int argc, char** argv)
{
	if(argc != 2)
	{
		cout << "Usage: " << argv[0] << " port\n";
		return EXIT_FAILURE;
	}

	int port = atoi(argv[1]);
	char rcv_buff[MAX_BUFF_SIZE] = {};
	string rcvStr, inputStr;
	
	// Create and init the listen socket
	Csocket server;
	int listenfd = server.InitServer(port);	
	int connfd, receive;
	
	int num_ready;
        struct epoll_event events[MAX_EPOLL_EVENTS];
        int epfd;
        epfd = epoll_create(1);
        struct epoll_event event;
        event.events = EPOLLIN; //Append "|EPOLLOUT" for write events as well
        event.data.fd = 0;
        epoll_ctl(epfd, EPOLL_CTL_ADD, 0, &event);
        event.data.fd = listenfd;
        epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &event);
	
	cout << "===== WELCOME TO CHATROOM =====\n";
	printCommands();
	cout << endl;

	while(1)
	{
		num_ready = epoll_wait(epfd, events, MAX_EPOLL_EVENTS, 30000);
		
		for(int i = 0; i < num_ready; i++)
		{
			if(events[i].data.fd == listenfd)
			{
				// Connection events
				connfd = server.Accept();
				if(connfd == -1)
				{
					// Error	
				}
				else
				{
					// Allocate new client
					CClient* client = new CClient;
					client->setSockfd(connfd);

					event.data.fd = connfd; 
					epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &event);

					if(handleClientName(client) == -1 )
					{
						cout << "Error handling clients names\n";
					}
				}
			}
			else if(events[i].data.fd == 0)
			{
				// Input events
				cout << "> ";
				getline(cin, inputStr);

				if (handleInput(inputStr) == -1)
				{
					cout << "Error: handling input\n";
				}
			}
			else
			{
				receive = server.ReceiveMessage(events[i].data.fd, rcv_buff);
				// Receive events
				if (receive == -1)
				{
					cout << "Error: receiving  message\n";
					return -1;
				}
				
				else if(receive == 0)
				{
					it_on = find_if(OnlineClients.begin(), OnlineClients.end(), FindBySockfd(events[i].data.fd));
					cout << (*it_on)->getClientName() << " exited." << endl;
					close(events[i].data.fd);
					OnlineClients.erase(it_on);
					delete *it_on;
					continue;
				}
				else
				{}

				rcvStr = rcv_buff;

				if( handleReceiveMsg(rcvStr, events[i].data.fd) == -1)
				{
					cout << "Error: handle receive message\n" << endl;
				}	
				bzero(rcv_buff, MAX_BUFF_SIZE);
			}
		}
	}
	
	
	return 0;
}
