#include "protocol/tmsxx.h"
#include <string.h>
#include "ep_app.h"
#include "stdio.h"
#include "epollserver.h"
#include "tms_app.h"
// #include "tmsxxdb.h"
#include "sys/wait.h"
// #include <strings.h>



#include "common/global.h"
#include "otdr_ch/hb_spi.h"
#ifdef __cplusplus
extern "C" {
#endif

extern int do_channal(void *ptr, int argc, char **argv);
extern int do_slot(void *ptr, int argc, char **argv);
extern int do_net(void *ptr, int argc, char **argv);

int g_cu_socket = 0;


#define SUBNET_1_XX (0x80)
#define SUBNET_0_XX (0x81)

#define SLOT_MASK (0xF0)
int g_slot = 0 & SLOT_MASK, g_slot_last;
int g_subnet = SUBNET_1_XX, g_subnet_last;

struct _tagSpiDev spiDev;

void NotifyCU(int fd)
{
	if (g_cu_socket == fd) {
		g_cu_socket = 0;
		system("echo 0 > /sys/class/leds/leda/brightness");
	}
}


/**
 * @brief	穷举所有网络连接并发送心跳，自连接同样发送，自身不处理心跳
 * @param	null
 * @retval	null
 * @remarks
 * @see
 */

int epcb_Tick(struct ep_con_t *ppconNode, void *ptr)
{
	tms_Tick(ppconNode->sockfd, NULL);
}
void *ThreadConnectCU(void *arg)
{
	// struct tmsxx_app *ptmsapp;
	// struct tms_context *pcontext;
	struct ep_t *pep = (struct ep_t *)arg;
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
	g_slot_last = g_slot;
	g_subnet_last = g_subnet;
	// initial_spi_dev(&spiDev, "/dev/spidev1.0", 0, 8, 0, 20000000);
	int32_t t_slot, t_subnet;
	while(1) {
#if 0
		if (g_cu_socket != 0) {
			tms_Tick(client.sockfd, NULL);
			sleep(5);
			continue;
		}

		if (0 == ep_Connect(pep, &client, "192.168.0.200", 6000) ) {
			system("echo 1 > /sys/class/leds/leda/brightness");
			g_cu_socket = client.sockfd;
		}
#endif
		// else if (0 == ep_Connect(pep,&client, "192.168.1.200", 6000) ) {
		// 	g_cu_socket = client.sockfd;
		// 	system("echo 1 > /sys/class/leds/leda/brightness");
		// }
		// else if (0 == ep_Connect(pep,&client, "192.168.1.251", 6000) ) {
		// 	g_cu_socket = client.sockfd;
		// 	system("echo 1 > /sys/class/leds/leda/brightness");
		// }
#if 0
		g_slot = do_slot(NULL, 0, NULL);
		g_subnet = do_net(NULL, 0, NULL);

		get_net_flag(&spiDev, &t_subnet);

		get_dev_slot(&spiDev, &t_slot);
		printf("slot %x %x   subnet %x %x\n\n", g_slot , t_slot, g_subnet, t_subnet);
		if (g_slot != g_slot_last || g_subnet != g_subnet_last) {

			if (g_subnet == SUBNET_0_XX) {
				printf("IP 192.168.0.%d\n",
				       (((~SLOT_MASK) & g_slot)) + 201);

			}
			else {
				printf("IP 192.168.1.%d\n",
				       (((~SLOT_MASK) & g_slot)) + 201);
			}
			// TODO 设置IP
			g_slot_last = g_slot;
			g_subnet_last = g_subnet;
		}


		sleep(1);
#endif
		// 自动重链201
		if (g_201fd == 0) {
			tms_connect();
		}
		// ep_Ergodic(&ep, epcb_Tick, NULL);
		sleep(15);
	}


	return NULL;
}

#ifdef __cplusplus
}
#endif
