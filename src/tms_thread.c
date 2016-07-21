#include "protocol/tmsxx.h"
#include <string.h>
#include "ep_app.h"
#include "stdio.h"
#include "epollserver.h"
#include "tms_app.h"
// #include "tmsxxdb.h"
#include "sys/wait.h"
// #include <strings.h>

#ifdef __cplusplus
extern "C" {
#endif


int g_cu_socket = 0;
void NotifyCU(int fd)
{
	if (g_cu_socket == fd) {
		g_cu_socket = 0;
		system("echo 0 > /sys/class/leds/leda/brightness");
	}
}

void *ThreadConnectCU(void *arg)
{
	// struct tmsxx_app *ptmsapp;
	// struct tms_context *pcontext;
	struct ep_t *pep = (struct ep_t*)arg;
	struct ep_con_t client;

	int server_fd;
	// int server_cnt;
	uint32_t server_addr;
	struct glink_addr gl_addr;
	bzero(&client, sizeof(struct ep_con_t));


	usleep(3000000);//延时3s，防止x86下efence奔溃

	gl_addr.pkid = 0;
	gl_addr.src = TMS_DEFAULT_LOCAL_ADDR;

	
	struct tmsxx_app *ptapp = (struct tmsxx_app *)client.ptr;
	while(1) {
		if (g_cu_socket != 0) {
			tms_Tick(client.sockfd, NULL);
			sleep(5);
			continue;
		}
		
		if (0 == ep_Connect(pep,&client, "192.168.0.200", 6000) ) {
			system("echo 1 > /sys/class/leds/leda/brightness");
			g_cu_socket = client.sockfd;	
		}
		
		// else if (0 == ep_Connect(pep,&client, "192.168.1.200", 6000) ) {
		// 	g_cu_socket = client.sockfd;	
		// 	system("echo 1 > /sys/class/leds/leda/brightness");
		// }
		// else if (0 == ep_Connect(pep,&client, "192.168.1.251", 6000) ) {
		// 	g_cu_socket = client.sockfd;	
		// 	system("echo 1 > /sys/class/leds/leda/brightness");
		// }
		sleep(1);
	}

	return NULL;
}

#ifdef __cplusplus
}
#endif
