#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>//获取下载时间
#include <sys/stat.h>//stat系统调用获取文件大小
#include <fcntl.h>//open系统调用
#include <time.h>

int thread_flag = 0;

struct header_inf{
	int reponse_inf;
	char content_type[128];
	int content_length;
};

const char *http_head[] = { "http://", "https://", NULL };

int tty_len;

char *progress_bar;

static const struct option longopts[] =
{
	{ "thread", 1, NULL, 't' },
	{ "help", 0, NULL, 'h' },
	{ "url", 1, NULL, 'u' },
	{ "file", 1, NULL, 'f' },
	{ NULL, 0, NULL, 0 }
};

void resolve_url(char* url, char* host, int *port, char* file_name);

struct header_inf get_header_inf(char* response);

long get_file_size(char* filename);

void get_ip(char *host_name, char *ip_addr);

int check_server(struct header_inf);

int Socket_connect(char *url, char *host, char *ip_addr, int port, long file_now, long file_end);

char *find_end(int sock);

void merge_tmp(char *file_path, char *file_name, int num);

void diect();

void download_file(int sock, char* file_name, long content_length);

void download_file_section(int sock, char *file_name, char *url, char *host, char* ip_addr, int port, long file_now, long file_end, int num, int bar_len);
