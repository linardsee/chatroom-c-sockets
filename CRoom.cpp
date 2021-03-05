#include "CRoom.h"

void CRoom::setRoomName(const string theName)
{
	roomName = theName;
}

string CRoom::getRoomName(void)
{
	return roomName;
}

int CRoom::getNumOfMembers(void)
{
	return numOfMembers;
}
