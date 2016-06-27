#include <minishell_core.h>

#include <stdio.h>
#include <protocol/SPICommand.h>
static int do_alarm(void *ptr, int argc, char **argv);
static int do_test(void *ptr, int argc, char **argv);
static int do_request(void *ptr, int argc, char **argv);
static int do_fpga_get(void *ptr, int argc, char **argv);
static int do_quit_system(void *ptr, int argc, char **argv);
static int do_disappear(void *ptr, int argc, char **argv);
static int do_appear(void *ptr, int argc, char **argv);
static int do_channal(void *ptr, int argc, char **argv);
static int do_slot(void *ptr, int argc, char **argv);
static int do_net(void *ptr, int argc, char **argv);



struct cmd_prompt boot_fpga_root[];
struct cmd_prompt boot_alarm[];
struct cmd_prompt boot_fpga_get[];



struct cmd_prompt boot_fpga_root[] = {
	PROMPT_NODE(boot_alarm   ,      do_alarm,
	(char *)"alarm"  ,
	(char *)"connect something",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_test,
	(char *)"test"  ,
	(char *)"to do ",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_request,
	(char *)"request"  ,
	(char *)"close connect",
	(int)  NULL),
	PROMPT_NODE(boot_fpga_get   ,      do_fpga_get,
	(char *)"get"  ,
	(char *)"Config Hostname",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_quit_system,
	(char *)"quit"  ,
	(char *)"Exit from current command view",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_alarm[] = {
	PROMPT_NODE(NULL    ,      do_disappear,
	(char *)"disappear"  ,
	(char *)"todo",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_appear,
	(char *)"appear"  ,
	(char *)"todo",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};
struct cmd_prompt boot_fpga_get[] = {
	PROMPT_NODE(NULL    ,      do_channal,
	(char *)"ch"  ,
	(char *)"all channal",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_slot,
	(char *)"slot"  ,
	(char *)"get slot",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_net,
	(char *)"net"  ,
	(char *)"get net",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};



static int do_alarm(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_test(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}


#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static const char *device = "/dev/spidev1.0";
static int do_request(void *ptr, int argc, char **argv)
{

	return 0;
}

static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 1000000;
static uint16_t delay  = 0;

static int do_fpga_get(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	
	if (argc >= 2 && memcmp(argv[1], "ch", strlen(argv[1])) == 0) {
		do_channal(ptr, argc, argv);
	}
	else if (argc >= 2 && memcmp(argv[1], "slot", strlen(argv[1])) == 0) {
		do_slot(ptr, argc, argv);
	}
	return 0;
}

static int do_quit_system(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	sh_editpath("");
	sh_up_prompt_level();

	return 0;
}

static int do_disappear(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);

	return 0;
}

static int do_appear(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}

static int do_channal(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);

	char tx[7];
	char rx[7];

	CmdSPIGetPipeNum(tx);


	struct spi_ioc_transfer array[8];

	int fd, ret;
	fd = open(device, O_RDWR);
	if (fd == NULL) {
		printf("open file %s error\n", device);
		return 0;
	}
	// transfer(fd);

	for (int i = 0; i < ARRAY_SIZE(tx);i++) {
		array[i].tx_buf = (unsigned long)tx + i;
		array[i].rx_buf = NULL;//(unsigned long)rx;
		array[i].len = 1;
		array[i].delay_usecs = 0;
		array[i].speed_hz = speed;
		array[i].bits_per_word = bits;
	}


	array[7].tx_buf = NULL;
	array[7].rx_buf = (unsigned long)rx;
	array[7].len = 7;
	array[7].delay_usecs = 0;
	array[7].speed_hz = speed;
	array[7].bits_per_word = bits;

	ret = ioctl(fd, SPI_IOC_MESSAGE(8), &array[0]);
	if (ret == 1) {
		perror("can't send spi message");
		return 0;
	}
	for (ret = 0; ret < ARRAY_SIZE(rx); ret++) {
		if (!(ret % 6)) {
			puts("");
		}
		printf("%.2X ", rx[ret]);
	}
	close(fd);

	return 0;
}

static int do_slot(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);

	char tx[7];
	char rx[7];

	CmdSPIGetUnitNumber(tx);


	struct spi_ioc_transfer array[8];

	int fd, ret;
	fd = open(device, O_RDWR);
	if (fd == NULL) {
		printf("open file %s error\n", device);
		return 0;
	}
	// transfer(fd);

	for (int i = 0; i < ARRAY_SIZE(tx);i++) {
		array[i].tx_buf = (unsigned long)tx + i;
		array[i].rx_buf = NULL;//(unsigned long)rx;
		array[i].len = 1;
		array[i].delay_usecs = 0;
		array[i].speed_hz = speed;
		array[i].bits_per_word = bits;
	}


	array[7].tx_buf = NULL;
	array[7].rx_buf = (unsigned long)rx;
	array[7].len = 7;
	array[7].delay_usecs = 0;
	array[7].speed_hz = speed;
	array[7].bits_per_word = bits;

	ret = ioctl(fd, SPI_IOC_MESSAGE(8), &array[0]);
	if (ret == 1) {
		perror("can't send spi message");
		return 0;
	}
	for (ret = 0; ret < ARRAY_SIZE(rx); ret++) {
		if (!(ret % 6)) {
			puts("");
		}
		printf("%.2X ", rx[ret]);
	}
	close(fd);
	return 0;
}

static int do_net(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	return 0;
}




