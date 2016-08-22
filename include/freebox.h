#ifndef _FREE_BOX_H_
#define _FREE_BOX_H_

#include <stdbool.h>

extern int part_configx(char *str, char *nval, char *val, int maxlen);
extern int read_rawconfig(char *file, char *nval, char *val, int maxlen);
extern int netIpIsValid(char *ip);
extern int netMaskIsValid(char *subnet);
extern int netGwIsValid(char *ip, char *mask, char *gw);
extern int FormatIPConfig(
    char *interface,
    char *ip,
    char *mask,
    char *gw,
    char *mac);
extern int CheckIPSwitch(char *ip, char *mask);

extern void minishell_load(char *file, struct cmd_prompt *boot);

#include <netcard.h>
extern bool SetInterfaceInfo(char *interface, struct itifo *val);



// napt addr port mapaddr mapport
#ifndef CONFIG_NAPT_MAX
	#define CONFIG_NAPT_MAX (10)
#endif
struct napt_item {
	char  s_addr[32];
	short  s_port;
	char  m_addr[32];
	short  m_port;
};
struct napt_list {
	struct napt_item list[CONFIG_NAPT_MAX];
	int index;
};

extern struct napt_list g_napt;
extern int nat_save(struct napt_list *val);
int save_main(int argc, char **argv);
int set_netcard_main(int argc, char **argv);
extern void save_iptables();

extern int show_napt(struct napt_list *val);
void firewall_on(int on_off);
extern int recovery_main(int argc, char **argv);
#endif