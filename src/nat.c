// iptables -t nat -F
// iptables -t nat -A POSTROUTING -s 0/0 -j MASQUERADE
// iptables -t nat -A PREROUTING -d 192.168.2.6 -p tcp --dport 8080 -j DNAT --to 192.168.1.5:80
// iptables -t nat -A PREROUTING -d 192.168.2.6 -p tcp --dport 123 -j DNAT --to 192.168.1.5:23


// iptables -F
// iptables -A FORWARD -s 192.168.1.0/255.255.255.0 -d 192.168.1.0/255.255.255.0 -j ACCEPT
// iptables -A FORWARD -s 192.168.2.0/255.255.255.0 -d 192.168.2.0/255.255.255.0 -j ACCEPT
// iptables -A FORWARD -p tcp --dport 80 -j ACCEPT
// iptables -A FORWARD -p tcp --dport 23 -j ACCEPT
// iptables -A FORWARD -s 0.0.0.0/0 -d 192.168.1.0/255.255.255.0 -j DROP
#include <autoconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freebox.h>
#include <net/if.h>
#include <unistd.h>
#include <netinet/in.h>
#include <minishell_core.h>

#ifdef CONFIG_DEF_ANY_CFG_DEBUG
	#define CONFIG_DEF_IPTABLE_CFG_DIR                              "iptables"
	#define CONFIG_DEF_FIREWALL_CFG                                 "./iptables/firewall.conf"
	#define CONFIG_DEF_NAPT_CFG                                     "./iptables/napt.conf"
	#define CONFIG_DEF_DNAT_CFG                                     "./iptables/dnat.conf"
	#define CONFIG_DEF_SNAT_CFG                                     "./iptables/snat.conf"
#endif



struct napt_list g_napt;

int napt_clear(int argc, char **argv)
{
	// printf("%s()\n", __FUNCTION__);
	system("iptables -t nat -F");
	g_napt.index = 0;
	// system("iptables -t nat -A POSTROUTING -s 0/0 -j MASQUERADE");
	return 0;
}


bool IsDigital(char *str)
{
	while(*str) {
		if ((*str < '0' || *str > '9') && *str != '\n') {
			return false;
		}
		str++;
	}
	return true;
}

int nat_save(struct napt_list *val)
{
	printf("%s()\n", __FUNCTION__);
	FILE *fp;
	struct napt_item *item;

	fp = fopen("/etc/nat-setting", "wb");
	fprintf(fp, "# NAT NAPT\n");
	fprintf(fp, "nat clear\n");
	item = &val->list[0];
	for (int i = 0; i < val->index; i++) {
		fprintf(fp, "nat napt %s %d\t\t%s %d\n",
		        item->s_addr,
		        item->s_port,
		        item->m_addr,
		        item->m_port);
		item++;
	}

	fclose(fp);
	return 0;
}
int show_napt(struct napt_list *val)
{
	struct napt_item *item;
	item = &val->list[0];
	printf("# NAT NAPT\n");
	for (int i = 0; i < val->index; i++) {
		printf("nat napt %s:%d\t\t%s:%d\n",
		        item->s_addr,
		        item->s_port,
		        item->m_addr,
		        item->m_port);
		item++;
	}
	printf("\n\n");
}
void minishell_load(char *file, struct cmd_prompt *boot)
{
	FILE *fin = NULL;
	char *pret = (char *)1;
	char strinput[2048];
	int ret = 0, len;
	// 读取打开配置文件
	fin = fopen( file, "r");
	if (NULL == fin) {
		printf("Error: can't open config file :%s.\nCheck path\n", file);
		return ;
	}

	// 逐行读取配置信息

	struct sh_detach_depth depth;
	char *cmd[12];

	sh_whereboot(boot);
	depth.cmd = cmd;
	depth.len = 12;
	depth.seps = (char *)" \t\n";

	while(NULL != pret) {
		pret = fgets(strinput, 2048, fin);

		len = strlen(strinput);
		sh_analyse_ex(strinput, len, &depth, NULL);

		if (ret) {
			break;
		}
	}

	fclose(fin);

}
int napt_main(int argc, char **argv)
{
	// nat napt x.x.x.x n x.x.x.x n
	if (argc < 6) {
		printf("syntax error\n");
		return 0;
	}
	short port, mapport;
	char  *addr, *mapaddr;
	char strout[256];

	addr    = argv[2];
	port    = atoi(argv[3]);
	mapaddr = argv[4];
	mapport = atoi(argv[5]);

	
	if (0 == netIpIsValid(addr) || 0 == netIpIsValid(mapaddr)) {
		printf("\tError IP\n");
		return -1;
	}
	else if(IsDigital(argv[3]) == false || IsDigital(argv[5]) == false) {
		printf("\tError Port\n");
		return -1;
	}
	

	snprintf(strout, 256, "iptables -t nat -A PREROUTING -d %s -p tcp --dport %d -j DNAT --to %s:%d",
	         addr, port, mapaddr, mapport);

	if (g_napt.index < CONFIG_NAPT_MAX) {
		struct napt_item *item;
		item = &g_napt.list[g_napt.index];
		strcpy(item->s_addr, addr);
		item->s_port = port;
		strcpy(item->m_addr, mapaddr);
		item->m_port = mapport;
		g_napt.index++;
		system(strout);
	}
	else {
		printf("NAPT Full!\n");
	}



}
int nat_main(int argc, char **argv)
{
	// printf("%s()\n", __FUNCTION__);

	// return 0;

	// if (argv[1][0] ==  'n') {
	// 	napt_main(argc, argv);
	// }
	// else if (argv[1][0] ==  'c') {
	// 	napt_clear(argc, argv);
	// }
	// else if (argv[1][0] ==  's') {
	// 	nat_save(&g_napt);
	// }
	// return 0;
	if (strcmp(argv[1], "napt") == 0) {
		napt_main(argc, argv);
	}
	else if (strcmp(argv[1], "clear") == 0) {
		napt_clear(argc, argv);
	}
	else if (strcmp(argv[1], "save") == 0) {
		nat_save(&g_napt);
	}
	// else if (strcmp(argv[1], "load") == 0) {
	// 	minishell_load("/etc/nat-setting", );
	// }


	return 0;
}