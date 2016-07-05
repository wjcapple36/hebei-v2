#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ramlog.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
const char def_rampath  [] = "/dev/shm/log";
const char def_diskpath  [] = "./log";
const char def_prefix[] =  "log-";



#ifdef _DEBUG
	#define dbg(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
	#define dbg(fmt, ...)
#endif

/**
 * @brief	找到文件最新的“几个”文件
 * @param	null
 * @retval	找到的文件数，最多 MAX_LOG
 * @remarks	仅将最新文件的ID写入 last_id

ls log -l
-rwxr-xr-x 1 root root 3 2016-04-15 09:11 log1
-rwxr-xr-x 1 root root 3 2016-04-15 09:14 log2
-rwxr-xr-x 1 root root 3 2016-04-15 09:16 log3
-rwxr-xr-x 1 root root 3 2016-04-15 09:18 log4
-rwxr-xr-x 1 root root 3 2016-04-15 09:20 log5

ls log -tr | grep log
log1
log2
log3
log4
log5

ls log -tr | grep log | tail -n 3
log3
log4
log5
 */
int _rl_findlast(struct ramlog *val, char *dir)
{

	char cmd[1024];
	char filter[32];
	FILE *stream;
	int i;

	// 在日志目录下搜索所有日志文件，并以时间排序
	snprintf(cmd, 1024,
	         "cd %s;"
	         "ls  | grep '%s'|"
	         "tail -n %d", dir, val->prefix, MAX_LAST_LOG);
	printf("cmd %s\n", cmd);

	stream = popen(cmd, "r");
	if (stream == NULL) {
		perror("_rl_findlast() popen()");
		return 0;
	}
	// fread(cmd , 1, 1024, stream);
	// printf("out %s\n", cmd);
	// return 0;
	// 遍历所有收到的文件名，并记录ID
	// snprintf(filter, 32, "%s%%d", val->prefix);
	snprintf(filter, 32, "%s%s-%%d", val->prefix, val->tm);
	for (i = 0; i < MAX_LAST_LOG; i++) {

		fgets(val->last_id[i], 1024, stream);
		if (feof(stream)) {
			break;
		}
		// strcpy(cmd, "log20160415-14-49-41-3");
		// sscanf(cmd, filter, &val->last_id[i]);
		// dbg("[%s] [%s] id = %d\n", cmd, filter, val->last_id[i]);
		int len;
		len = strlen(val->last_id[i]);
		val->last_id[i][len-1] = '\0';
		dbg("[%s]\n", val->last_id[i]);
	}
	printf("i = %d\n", i);
	// if (i == 0) {
	// 	val->curid = val->last_id[i];
	// }
	// else {
	// 	val->curid = val->last_id[i - 1];
	// }
	pclose(stream);


	return 0;
}

/**
 * @brief	清除文件夹，删除大多数日志文件，仅保留最新文件
 * @param	null
 * @retval	null
 * @remarks
 */
int _rl_rm_except_last(struct ramlog *val, char *dir)
{
	char cmd[1024];
	int i;
	char filelist[256] = "";

	for (i = 0; i < MAX_LAST_LOG; i++) {
		if (val->last_id[i]) {
			if (i != 0) {
				strcat(filelist, "\\|");
			}
			strcat(filelist, val->last_id[i]);
		}
		else {
			break;	
		}
		
	}
	// 在日志目录下搜索所有日志文件，并以时间排序
	snprintf(cmd, 1024,
	         "cd %s;"
	         "ls  | grep -v '%s'|"
	         "xargs rm -f", dir, filelist);
	printf("--------cmd %s\n", cmd);
	system(cmd);
	// exit(0);

	return 0;
}
#include <time.h>
void _rl_tm(struct ramlog *val)
{
	time_t t;
	struct tm *local; //本地时间

	t = time(NULL);
	local = localtime(&t); //转为本地时间
	strftime(val->tm, 20, "%Y%m%d-%H-%M-%S", local);
}
/**
 * @brief	清除所有ram文件
 * @param	null
 * @retval	null
 * @remarks	
 * @see	
 */

void _rl_rm_ram(struct ramlog *val)
{
	char cmd[1024];
	snprintf(cmd, 1024, "rm %s/*", val->rampath);
	system(cmd);
}

/**
 * @brief	将ram文件拷贝到disk（当主进程崩溃时子进程自动完成）
 * @param	null
 * @retval	null
 * @remarks	
 * @see	
 */

void _rl_ram2disk(struct ramlog *val)
{
	// return ;
	char cmd[1024];
	snprintf(cmd, 1024, "cp %s/* %s", val->rampath, val->diskpath);
	system(cmd);
	printf("val->curid %d\n", val->curid);
}

/**
 * @brief	如果日志目录过大则清除部分
 * @param	null
 * @retval	null
 * @remarks	
 * @see	
 */

int rl_dirlimitsize(struct ramlog *val, char *dir)
{
	char cmd[1024];
	char str[32];
	FILE *stream;
	int dirsize;

	snprintf(cmd, 1024, "du -b %s", dir);
	printf("check dir %s\n", cmd);
	stream = popen(cmd, "r");
	if (stream == NULL) {
		return 0;
	}
	fgets(str,  32, stream);
	pclose(stream);



	dirsize = atoi(str);
	printf("dirsize = %d s_total %ld %s\n", dirsize , val->s_total, dir);
	if (dirsize > val->s_total) {
		_rl_rm_except_last(val, dir);
	}
	
	return 0;
}

/**
 * @brief	
 * @param	null
 * @retval	null
 * @remarks	
 * @see	
 */

int rl_init(struct ramlog *val,
            char *rampath, char *diskpath,
            int s_log, int s_total)
{
	bzero(val, sizeof(struct ramlog));


	if (NULL == rampath) {
		val->rampath     = (char*)def_rampath;
	}
	else {
		val->rampath     = (char*)rampath;
	}
	if (NULL == diskpath) {
		val->diskpath    = (char*)def_diskpath;
	}
	else {
		val->diskpath    = (char*)diskpath;
	}
	val->prefix          = (char*)def_prefix;

	if (s_log < 4 * _1K ) {
		val->s_log        = 4 * _1K;
	}
	else {
		val->s_log	= s_log;
	}
	if (s_total < val->s_log * MAX_LAST_LOG) {
		val->s_total = 4 * _1K * MAX_LAST_LOG;
	}
	else {
		val->s_total = s_total;
	}


	mkdir(val->rampath, 777);
	mkdir(val->diskpath, 777);

	_rl_tm(val);
	_rl_rm_ram(val);
	_rl_findlast(val, val->diskpath);
	rl_dirlimitsize(val, val->diskpath);
	
	// printf("logid %d\n", val->curid);
	
	val->curid = 0;
	snprintf(val->name, 256, "%s/%s%s-%d", val->rampath, val->prefix, val->tm, val->curid);
	printf("val->name %s\n" , val->name);

	
	pid_t fpid = fork();
	if (fpid < 0) {
		perror("fork");
	}
	else if (fpid == 0) {
		printf("child %d %d\n" , getpid(), getppid());
		while (getppid() != 1) {
			sleep(1);
		}
		printf("get sig\n");
		// TODO copy ramfs to disk
		_rl_ram2disk(val);

		exit(0);
	}
	return 0;
}


/**
 * @brief	写日志
 * @param	null
 * @retval	null
 * @remarks
 * @see
 */
int rl_onefile(struct ramlog *val, char *str, int n)
{
	FILE *fd;
	dbg("\nopen file %s size %d\n", val->name, n);
	fd = fopen(val->name, "a+");
	if (fd == NULL) {
		dbg("err");
		return 0;
	}
	int len, byte;
	byte = fwrite(str,  1, n, fd);

	dbg("byte = %d\n", byte);
	len = ftell(fd);
	fclose(fd);
	// sleep(1);
	return len;
}

int rl_multifile(struct ramlog *val, char *str, int n)
{
	int byte, offset = 0;

	if (val->s_cur + n > val->s_log) {
		byte = val->s_log - val->s_cur;
		val->s_cur = rl_onefile(val, str, byte);
		offset += byte;
		n -= byte;
		while(n > 0) {
			if (n > val->s_log) {
				byte = val->s_log;
			}
			else {
				byte = n;
			}
			val->curid++;
			snprintf(val->name, 256, "%s/%s%s-%d", val->rampath, val->prefix, val->tm, val->curid);

			val->s_cur = rl_onefile(val, str + offset, byte);
			offset += byte;
			n -= byte;
		}
	}
	else {
		val->s_cur = rl_onefile(val, str, n);
	}
	return val->s_cur;
}



#include <stdarg.h>
int
rl_snprintf (struct ramlog *val, char *s, size_t maxlen, const char *format, ...)
{
  va_list arg;
  int done;

  va_start (arg, format);
  done = vsnprintf (s, maxlen, format, arg);
  va_end (arg);

  rl_multifile(val, s, done);
  return done;
}