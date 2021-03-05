#include <iostream>
#include <string>


using namespace std;

class CRoom
{
        public:
                CRoom(){}                
		~CRoom(){};
		void setRoomName(const string theName);
		string getRoomName(void);
        	int getNumOfMembers(void);

		void operator -- ()
		{
			numOfMembers--;
		}	

		void operator ++ ()
		{
			numOfMembers++;
		}
	private:
                string roomName;
		int numOfMembers;
};

