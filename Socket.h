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
#include <sys/time.h>//��ȡ����ʱ��
#include <sys/stat.h>//statϵͳ���û�ȡ�ļ���С
#include <fcntl.h>//openϵͳ����

//return sock,if sock <0,error 
struct header_inf{
	int reponse_inf;
	char content_type[128];
	int content_length;
};

//void resolve_url(char* url, char* host, int &port, char* file_name);

//struct header_inf get_header_inf(char* &response);