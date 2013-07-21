#include <vector>
#include <time.h>


//structura folosita pentru a retine informatii despre un client
struct client{
	int id;
	int port;
	long c_time;
	char *nume;
	char *addr;
	std::vector<char*> fisiere;
};

//mesajului care se transmite prin socket
struct msg {
	int type, info;
	char payload[1024];
};
