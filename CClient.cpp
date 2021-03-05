
#include "CClient.h"

CClient::CClient()
{
}

CClient::CClient(string clName)
{
	clientName = clName;
}

int CClient::getUid(void)
{
	return uid;
}

void CClient::setUid(const int userId)
{
	uid = userId;
}

int CClient::getSockfd(void)
{
	return sockfd;
}

void CClient::setSockfd(const int socketfd)
{
	sockfd = socketfd;
}

string CClient::getClientName(void)
{
	return clientName;
}

void CClient::setClientName(const string theName)
{
	clientName = theName;
}

string CClient::getLastRoom(void)
{
	return roomName;
}

void CClient::setLastRoom(const string theRoom)
{
	roomName = theRoom;
}

void CClient::setAddress(struct sockaddr_in theAddress)
{
	address = theAddress;
}
