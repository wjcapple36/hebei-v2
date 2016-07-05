
#include <stdio.h>
#include <sys/time.h>
#include <stdio.h>
#include <math.h>

// #include <ramlog.h>
struct timeval tpstart, tpend;
float timeuse;
// 记录开始时间
void ResetConst()
{
	gettimeofday(&tpstart, NULL);
}
// 打印从调用ResetConst到调用PrintCost之间代码运行时间
float PrintCost()
{

	gettimeofday(&tpend, NULL);
	timeuse = 1000000 * (tpend.tv_sec - tpstart.tv_sec) +
	          tpend.tv_usec - tpstart.tv_usec;
	timeuse /= 1000000;
	printf("Used Time:%f\r\n", timeuse);
	return timeuse;

}


char strout[1024 * 1024];
int index;
struct trace_cache {
	char *strout;	///< 输出执法车
	int offset;		///< 输出字符串偏移
	int empty;		///< 最大空余
	int limit;		///< 填充极限
};

struct trace_cache cache;
struct trace_cache *ptc = &cache;


void writecache( char *str, int len)
{
	int ret;
	// ret = snprintf(ptc->strout + ptc->offset, ptc->empty - ptc->offset,"%s",str);
	// printf("[%s] %d\n", str, ret);
	// ptc->offset += ret;
	// if (ptc->offset >= cache.limit) {
	// 	ptc->offset = 0;
	// }
	// ret = strlen(str);

	memcpy(ptc->strout + ptc->offset, str, len);
	ptc->offset += len;
	if (ptc->offset > ptc->limit) {
		ptc->offset = 0;
	}
	// memcpy(ptc->strout, str, len);
}
int main(int argc, char **argv)
{
	char tmp[256];
	int i, len;
	cache.strout = strout;
	cache.limit = 1024 * 1024;
	cache.offset = 0;
	cache.empty = 1024 * 1024;

	i = sprintf(tmp, "%s", "kabcd");
	printf("i = %d\n", i);
	ResetConst();
	for ( i = 0; i < 1000; i++) {
		len = strlen(argv[1]);
		writecache(argv[1], len);
		len = strlen(argv[2]);
		writecache(argv[2], len);



		// len = sprintf(tmp, "%s", argv[1]);
		// writecache(tmp, len);
		// len = sprintf(tmp, "%s", argv[2]);
		// writecache(tmp, len);
	}
	PrintCost();

	FILE *fd;
	fd = fopen("/dev/shm/af", "a+");

	ResetConst();
	for ( i = 0; i < 1000; i++) {
		// printf("%s", argv[1]);
		// printf("%s", argv[2]);
		fprintf(fd,"%s", argv[1]);
		fprintf(fd,"%s", argv[2]);
	}
	fclose(fd);
	PrintCost();

	return 0;
	// printf("strout %s\n", strout);
}
