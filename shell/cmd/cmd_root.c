#include <autoconfig.h>
#include <minishell_core.h>

#include <stdio.h>

static int do_fpga(void *ptr, int argc, char **argv);
static int do_net(void *ptr, int argc, char **argv);
static int do_logicapp(void *ptr, int argc, char **argv);
static int do_quit_system(void *ptr, int argc, char **argv);


struct cmd_prompt boot_root[];

#ifdef CONFIG_CMD_FPGA
	extern struct cmd_prompt boot_fpga_root[];
#endif

#ifdef CONFIG_CMD_SERVER
	extern struct cmd_prompt boot_epollserver_root[];
#endif

extern struct cmd_prompt boot_logicapp_root[];


struct cmd_prompt boot_root[] = {
#ifdef CONFIG_CMD_FPGA
	PROMPT_NODE(NULL   ,      do_fpga,
	(char *)"fpga"  ,
	(char *)"connect something",
	(int)  NULL),
#endif
#ifdef CONFIG_CMD_SERVER
	PROMPT_NODE(NULL   ,      do_net,
	(char *)"net"  ,
	(char *)"to do ",
	(int)  NULL),
#endif
	PROMPT_NODE(NULL   ,      do_logicapp,
	(char *)"logic"  ,
	(char *)"",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      do_quit_system,
	(char *)"quit"  ,
	(char *)"Exit from current command view",
	(int)  NULL),
	PROMPT_NODE(NULL    ,      NULL, (char *)NULL, (char *)NULL, (int *) NULL),
};


#ifdef CONFIG_CMD_FPGA


#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>

static uint8_t mode;
static uint8_t bits = 8;
static uint32_t speed = 20000000;
static uint16_t delay  = 0;
static const char *device = "/dev/spidev1.0";
static int do_fpga(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);

	// 设置 SPI 速度 ===========================
	int ret, fd;

	fd = open(device, O_RDWR);
	if (fd == NULL) {
		printf("open file %s error\n", device);
		return 0;
	}
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		printf("can't set spi mode");
	}
	printf("SPI_IOC_WR_MODE %d\n", mode);

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) {
		printf("can't get spi mode");
	}
	printf("SPI_IOC_RD_MODE %d\n", mode);
	/*
	 * bits per word
	 */
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("can't set bits per word");
	}
	printf("SPI_IOC_WR_BITS_PER_WORD %d\n", bits);

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
		printf("can't get bits per word");
	}
	printf("SPI_IOC_RD_BITS_PER_WORD %d\n", bits);

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		printf("can't set max speed hz");
	}
	printf("SPI_IOC_WR_MAX_SPEED_HZ %d\n", speed);


	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		printf("can't get max speed hz");
	}
	printf("SPI_IOC_RD_MAX_SPEED_HZ %d\n", speed);
	close(fd);

	// end 设置 SPI 速度 ===========================

	// 切换命令行
	sh_editpath("fpga");
	sh_down_prompt_level(boot_fpga_root);
	return 0;
}
#endif
#ifdef CONFIG_CMD_SERVER
static int do_net(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	sh_editpath("net");
	sh_down_prompt_level(boot_epollserver_root);
	return 0;
}
#endif

static int do_logicapp(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	sh_editpath("logic");
	sh_down_prompt_level(boot_logicapp_root);
	return 0;
}
static int do_quit_system(void *ptr, int argc, char **argv)
{
	printf("%s\n", __FUNCTION__);
	exit(0);
	return 0;
}


