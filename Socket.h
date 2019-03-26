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

//return sock,if sock <0,error 
struct header_inf{
	int reponse_inf;
	char content_type[128];
	int content_length;
};

//void resolve_url(char* url, char* host, int &port, char* file_name);

//struct header_inf get_header_inf(char* &response);