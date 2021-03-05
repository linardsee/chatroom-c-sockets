
#include <iostream>
#include <string>
#include <netinet/in.h>


using namespace std;

class CClient
{
        public:
		CClient();
                CClient(string clName);
                ~CClient(){};
                int getUid(void);
                void setUid(const int userId);
                int getSockfd(void);
                void setSockfd(const int socketfd);
                string getClientName(void);
                void setClientName(const string theName);
                string getLastRoom(void);
                void setLastRoom(const string theRoom);
                void setAddress(struct sockaddr_in theAddress);

        private:
                int uid, sockfd;
                string clientName, roomName;
                struct sockaddr_in address;
};

