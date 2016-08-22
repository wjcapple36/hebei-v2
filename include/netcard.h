#ifndef _NETCARD_H_
#define _NETCARD_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
struct itifo
{
	struct sockaddr_in addr;
	struct sockaddr_in dstaddr;
	struct sockaddr_in broadaddr;
	struct sockaddr_in netmask;
	char mac[18];
};
bool IsValidInterface(char *interface);
bool GetInterfaceInfo(char *interface, struct itifo *val);

void save_netcard();
#endif