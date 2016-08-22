/**
 ******************************************************************************
 * @file	netcard.c
 * @brief	Linux 双网卡平台自动切换网卡，网卡配置信息写入 /etc/ethX-setting，根据
 此配置为基础配置防火墙，路由


- 2015-09-30,Menglong Woo, MenglongWoo@aliyun.com
 	- brief

 * @attention
 *
 * ATTENTION
 *
 * <h2><center>&copy; COPYRIGHT </center></h2>
*/

#include "stdio.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "string.h"
#include "stdlib.h"
#include <stdbool.h>
#include <netcard.h>
#include <freebox.h>
#include <if.h>
#include <sys/ioctl.h>

bool IsValidInterface(char *interface)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[2048];
	int success = 0;

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (sock == -1) {
		return false;
	}

	ifc.ifc_len = sizeof(buf);
	ifc.ifc_buf = buf;
	if (ioctl(sock, SIOCGIFCONF, &ifc) == -1) {
		return false;
	}

	struct ifreq *it = ifc.ifc_req;
	const struct ifreq *const end = it + (ifc.ifc_len / sizeof(struct ifreq));
	char szMac[64];
	int count = 0;
	strcpy(ifr.ifr_name, interface);
	if (ioctl(sock, SIOCGIFFLAGS, &ifr) == 0) {
		return true;
	}
	close(sock);
	return false;
}

bool SetInterfaceInfo(char *interface, struct itifo *val)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[2048];
	int success = 0;

	struct sockaddr_in addr;

	if (IsValidInterface(interface) == false) {
		return false;
	}
	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(ifr.ifr_name, interface);
	// ifr.ifr_ifru.ifru_addr.sin_addr = 0x2233311;
	// perror("what:");
	struct sockaddr_in *p = (struct sockaddr_in *) & (ifr.ifr_addr);

	p->sin_family = AF_INET;
	// inet_aton( "192.168.3.2", &(p->sin_addr));
	addr.sin_addr.s_addr = inet_addr("192.168.3.22");
	inet_aton( inet_ntoa(val->addr.sin_addr), &(p->sin_addr));
	if (ioctl(sock, SIOCSIFADDR, &ifr) == 0) {
		printf("success\n");
	}

	perror("what:");
}

bool GetInterfaceInfo(char *interface, struct itifo *val)
{
	struct ifreq ifr;
	struct ifconf ifc;
	char buf[2048];
	int success = 0;

	if (IsValidInterface(interface) == false) {
		return false;
	}

	int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	strcpy(ifr.ifr_name, interface);

	if (ioctl(sock, SIOCGIFADDR, &ifr) == 0) {
		// struct sockaddr_in broadcast_addr;//广播地址
		memcpy(&val->addr, &ifr.ifr_ifru.ifru_addr, sizeof(struct sockaddr_in));
		// printf("addr %s ", inet_ntoa(val->addr.sin_addr));
	}

	if (ioctl(sock, SIOCGIFBRDADDR, &ifr) == 0) {
		// struct sockaddr_in broadcast_addr;//广播地址
		memcpy(&val->broadaddr, &ifr.ifr_ifru.ifru_broadaddr, sizeof(struct sockaddr_in));
		// printf("board %s ", inet_ntoa(val->broadaddr.sin_addr));
	}

	if (ioctl(sock, SIOCGIFNETMASK, &ifr) == 0) {
		// struct sockaddr_in broadcast_addr;//广播地址
		memcpy(&val->netmask, &ifr.ifr_ifru.ifru_netmask, sizeof(struct sockaddr_in));
		// printf("mask %s ", inet_ntoa(val->netmask.sin_addr));
	}

	if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
		unsigned char *ptr ;
		ptr = (unsigned char *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
		snprintf(&val->mac[0], 18, "%02X:%02X:%02X:%02X:%02X:%02X",
		         *ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *(ptr + 4), *(ptr + 5));
		// printf("mac %s \n", val->mac);
	}
	return true;
}
