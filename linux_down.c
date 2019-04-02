#include "Socket.h"
//#include <thread>
#include <omp.h>
#include<termios.h>
#include<sys/ioctl.h>
//using std::thread;


void resolve_url(char* url, char* host, int *port, char* file_name){//分离url中的数据
	int start_flag = 0;

	//printf("strlen:    %d\n", strlen(file_name));

	for (int i = 0; http_head[i] != NULL;++i){
		if (strstr(url, http_head[i]) != NULL)
			start_flag = strlen(http_head[i]);
	}
	int j = 0;
	for (int i = start_flag; url[i] != '/' && url[i] != '\0'; i++, j++)
		host[j] = url[i];
	host[j] = '\0';

	char *pos = strstr(host, ":");
	if (pos){
		(*port) = atoi(pos);
		host[pos - host] = '\0';
	}

	sprintf(host, "%s", url + start_flag);

	host[strstr(url + start_flag, "/") - (url + start_flag)] = '\0';
	
	j = 0;
	for (int i = start_flag; url[i] != '\0'; i++){
		if (url[i] == '/'){
			if (i != strlen(url) - 1)
				j = 0;
			continue;
		}
		else
			file_name[j++] = url[i];
	}
	file_name[j] = '\0';
}

struct header_inf get_header_inf(char* response){//建立结构体
	struct header_inf tmp;

	char *pos = strstr(response, "HTTP/");
	if (pos)//»ñÈ¡·µ»Ø´úÂë
		sscanf(pos, "%*s %d", &tmp.reponse_inf);

	pos = strstr(response, "Content-Type:");
	if (pos)//»ñÈ¡·µ»ØÎÄµµÀàÐÍ
		sscanf(pos, "%*s %s", tmp.content_type);

	pos = strstr(response, "Content-Length:");
	if (pos)//»ñÈ¡·µ»ØÎÄµµ³¤¶È
		sscanf(pos, "%*s %ld", &tmp.content_length);

	return tmp;
}

long get_file_size(char* filename){//计算文件长度
	struct stat tmp;
	if (stat(filename, &tmp) < 0)
		return 0;
	return (long)tmp.st_size;
}

void get_ip(char *host_name, char *ip_addr)//通过gethostbyname得到ip
{
	struct hostent *host = gethostbyname(host_name);//´Ëº¯Êý½«»á·ÃÎÊDNS·þÎñÆ÷
	if (!host)
	{
		ip_addr = NULL;
		return;
	}

	for (int i = 0; host->h_addr_list[i]; i++)
	{
		strcpy(ip_addr, inet_ntoa(*(struct in_addr*) host->h_addr_list[i]));
		break;
	}
}

int check_server(struct header_inf h){//检测服务器返回的头中的状态值
	if (h.reponse_inf == 200)
		return 1;
	if (h.reponse_inf == 206)
		return 2;
	else return 0;
}

int Socket_connect(char *url, char *host,char *ip_addr,int port,long file_now,long file_end){//处理socket的连接
	char ana_header[2048] = { 0 };
	if (file_now==0&&file_end==0)
		sprintf(ana_header, \
		"GET %s HTTP/1.1\r\n"\
		"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
		"Host: %s\r\n"\
		"Connection: keep-alive\r\n"\
		"\r\n"\
		, url, host);
	else{
		sprintf(ana_header, \
			"GET %s HTTP/1.1\r\n"\
			"Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8\r\n"\
			"User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537(KHTML, like Gecko) Chrome/47.0.2526Safari/537.36\r\n"\
			"Range: bytes=%ld-%ld\r\n"\
			"Host: %s\r\n"\
			"Connection: keep-alive\r\n"\
			"\r\n"\
			, url, file_now, file_end,host);
		if (thread_flag == 0){
			printf(">>>>>>>>>It's Download From Break Point!<<<<<<<<<\n");
			//printf(">>>>>>>>>So The No1.And No.2 Be Ignored!<<<<<<<<<\n");
		}
	}

	//if (thread_flag == 0)
	//printf("No.3 Create socket.\n");

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock < 0)
	{
		printf("create socket error: %d\n", sock);
		exit(-1);
	}

	//´´½¨IPµØÖ·½á¹¹Ìå
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(ip_addr);
	addr.sin_port = htons(port);

	//Á¬½ÓÔ¶³ÌÖ÷»ú
	//if (thread_flag == 0)
	//printf("No.4 Connecting.\n");
	int res = connect(sock, (struct sockaddr *) &addr, sizeof(addr));
	if (res == -1)
	{
		printf("Connect error: %d\n", res);
		exit(-1);
	}
	//if (thread_flag == 0)
	//printf("No.5 Sending http response.\n");
	write(sock, ana_header, strlen(ana_header));
	//printf("Header:%s\n\n", ana_header);
	return sock;
}

char *find_end(int sock){//找到服务器返回的http头的末尾，末尾之后就是文件了
	int mem_size = 4096;
	int length = 0;//已从socket中读取数据总长度
	int len;//正在从socket中读取出来的数据长度，一个个读取所以应该是1
	char *buf = (char *)malloc(mem_size * sizeof(char));
	char *response = (char *)malloc(mem_size * sizeof(char));

	while ((len = read(sock, buf, 1)) != 0)
	{
		if (length + len > mem_size)//不确定什么时候才能到http头文件结束，所以不能开固定化长度
		{
			//¶¯Ì¬ÄÚ´æÉêÇë, ÒòÎªÎÞ·¨È·¶¨ÏìÓ¦Í·ÄÚÈÝ³¤¶È
			mem_size *= 2;
			char * temp = (char *)realloc(response, sizeof(char)* mem_size);
			if (temp == NULL)
			{
				printf("No space\n");
				exit(-1);
			}
			response = temp;
			//free(temp);
			//temp = NULL;
		}

		buf[len] = '\0';
		strcat(response, buf);

		//ÕÒµ½ÏìÓ¦Í·µÄÍ·²¿ÐÅÏ¢
		int flag = 0;
		for (int i = strlen(response) - 1; response[i] == '\n' || response[i] == '\r'; i--, flag++);//计算末尾\r\n长度，最大为四时表示结束了，后面就是需要的文件
		if (flag == 4)//连续两个换行和回车表示已经到达响应头的头尾, 即将出现的就是需要下载的内容
			break;

		length += len;
	}
	free(buf);
	buf = NULL;
	return response;
}

void merge_tmp(char *file_path,char *file_name,int num){//合并tmp文件夹中的缓存
	int fd = open(file_name, O_CREAT | O_WRONLY | O_APPEND, S_IRWXG | S_IRWXO | S_IRWXU);
	if (fd < 0)
	{
		printf("Create file error!,check your storage space!\n");
		exit(0);
	}
	for (int i = 1; i < num+1; ++i){
		char tmp_path[9];
		strcpy(tmp_path, "./tmp/");
		sprintf(tmp_path + strlen(tmp_path), "%d", i);
		int tmp_fd = open(tmp_path, O_RDONLY);
		if (tmp_fd < 0)
		{
			printf("This File %s Open Error!!!\n",tmp_path);
			exit(0);
		}
		int mem_size = 8192;
		int buf_len = mem_size;

		char *buf = (char *)malloc(mem_size * sizeof(char));

		int tmp_size = get_file_size(tmp_path);
		int len = 0;
		int read_len = 0;

		while (read_len < tmp_size){
			len = read(tmp_fd, buf, buf_len);
			write(fd, buf, len);
			read_len += len;
			if (read_len == tmp_size)
				break;
		}
		remove(tmp_path);
		close(tmp_fd);
		free(buf);
		buf = NULL;
	}
}

void diect()
{
	fprintf(stderr,
		"-h --help 打印帮助信息\n"
		"-t --thread 指定线程数，例如-t 8,16以上的指定将无效,此方法将在当前目录下创造一个tmp文件夹以存放临时文件\n"
		"-u --url 指定url，必设置的参数\n"
		"-f --file 指定file文件名，如果没有此参数，默认为原名下载至当前目录\n"
		);
	exit(0);
};

void download_file(int sock, char* file_name, long content_length){//无断点的下载

	long down_length = 0;//¼ÇÂ¼ÒÑ¾­ÏÂÔØµÄ³¤¶È//¼ÇÂ¼Ò»´Î¶ÁÈ¡µÄÊ±¼äÆðµãºÍÖÕµã, ¼ÆËãËÙ¶È
	struct timeval t_start, t_end;//¼ÇÂ¼Ò»´Î¶ÁÈ¡µÄÊ±¼äÆðµãºÍÖÕµã, ¼ÆËãËÙ¶È
	int once_size = 8192;//»º³åÇø´óÐ¡8K
	int once = once_size;//ÀíÏë×´Ì¬Ã¿´Î¶ÁÈ¡8K´óÐ¡µÄ×Ö½ÚÁ÷
	int len;

	//´´½¨ÎÄ¼þÃèÊö·û

		int fd = open(file_name, O_CREAT | O_WRONLY, S_IRWXG | S_IRWXO | S_IRWXU);
		if (fd < 0)
		{
			printf("Create file error!,check your storage space!\n");
			exit(0);
		}

		long used_time = 0;
		int mem_size = 8192;
		int buf_len = mem_size;

		char *buf = (char *)malloc(mem_size * sizeof(char));

		while (down_length < content_length)
		{

			gettimeofday(&t_start, NULL); //»ñÈ¡¿ªÊ¼Ê±¼ä
			len = read(sock, buf, buf_len);
			write(fd, buf, len);
			gettimeofday(&t_end, NULL); //»ñÈ¡½áÊøÊ±¼ä

			if (t_end.tv_usec - t_start.tv_usec >= 0 && t_end.tv_sec - t_start.tv_sec >= 0)
				used_time += 1000000 * (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_usec - t_start.tv_usec);

			//printf("\rHas used %d%d%d%.4f secs to download file,%ld  has been download ", (double)used_time / 1000000,down_length);
			printf("\rHas used %.4f secs to download file , %.2f %% file, %.3f MB has been download.", (double)used_time / 1000000, (float)100 * down_length / content_length, (double)down_length / 1024.0 / 1024.0);
			fflush(stdout);

			down_length += len;//

			if (down_length == content_length){
				printf("\n");
				float down_length_kb = (float)down_length / 1024.00;
				float used_time_sec = (float)used_time / 1000000.00;
				printf("The average speed is: %.2f MB/s \n", down_length_kb/used_time_sec/1024.00);
				break;
			}
		}
		free(buf);
		buf = NULL;
}

void download_file_section(int sock, char *file_name, char *url, char *host, char* ip_addr, int port, long file_now,long file_end,int num,int bar_len){
	//有起始点和终结点的下载，基于HTTP1.1的Range

	shutdown(sock, 2);


	long down_length = 0;//¼ÇÂ¼ÒÑ¾­ÏÂÔØµÄ³¤¶È
	struct timeval t_start, t_end;//¼ÇÂ¼Ò»´Î¶ÁÈ¡µÄÊ±¼äÆðµãºÍÖÕµã, ¼ÆËãËÙ¶È
	int once_size = 8192;//»º³åÇø´óÐ¡8K
	int once = once_size;//ÀíÏë×´Ì¬Ã¿´Î¶ÁÈ¡8K´óÐ¡µÄ×Ö½ÚÁ÷
	int len;

	long need_down = file_end - file_now;

	if (num==0)
	printf("此段需下载的总长度为： %ld 已下载为： %ld \n", need_down, file_now);
	//printf("File continue Download!\n");

	sock = Socket_connect(url, host, ip_addr, port, file_now,file_end);

	char *response = find_end(sock);

	struct header_inf header_inf = get_header_inf(response);

	if (num==0)
	printf("状态码：  %d", header_inf.reponse_inf);

	if (check_server(header_inf) != 2){
		printf("It's not support to resume from break point\n");
		exit(-1);
	}

	int fd = open(file_name, O_WRONLY | O_APPEND|O_CREAT,S_IRWXG | S_IRWXO | S_IRWXU);
	if (fd < 0){
		printf("Open file Error!\n");
		exit(0);
	}
	long used_time = 0;
	int mem_size = 8192;
	int buf_len = mem_size;

	char *buf = (char *)malloc(mem_size * sizeof(char));

	while (down_length < need_down)
	{
		gettimeofday(&t_start, NULL); //»ñÈ¡¿ªÊ¼Ê±¼ä
		len = read(sock, buf, buf_len);
		write(fd, buf, len);
		gettimeofday(&t_end, NULL); //»ñÈ¡½áÊøÊ±¼ä



		//printf("\rHas used %d%d%d%.4f secs to download file,%ld  has been download ", (double)used_time / 1000000,down_length);
		if (num == 0){
			if (t_end.tv_usec - t_start.tv_usec >= 0 && t_end.tv_sec - t_start.tv_sec >= 0)
				used_time += 1000000 * (t_end.tv_sec - t_start.tv_sec) + (t_end.tv_usec - t_start.tv_usec);
			printf("\rHas used %.4f secs to download file , %.2f %% file, %.3f MB has been download.", (double)used_time / 1000000, (float)100 * down_length / need_down, (double)down_length / 1024.0 / 1024.0);
			fflush(stdout);
		}
		else{
			//printf("\nProrgress Bar: \n"); 
			{
				int k = down_length*bar_len / need_down;//2 10 5
				for (int i = 0; i < k + 1; ++i)
					progress_bar[bar_len*(num - 1) + i] = '=';
				printf("\r%s 线程%d正在下载", progress_bar,num);
				fflush(stdout);
			}
			//printf("\r%s", progress_bar);
		}

		down_length += len;//

		if (down_length == need_down){
			printf("\n");
			float down_length_kb = (float)down_length / 1024.00;
			float used_time_sec = (float)used_time / 1000000.00;
			printf("The average speed is: %.2f MB/s \n", down_length_kb / used_time_sec / 1024.00);
			break;
		}
	}
	free(buf);
	buf = NULL;
}


int main(int argc, char *argv[]){

	struct winsize size;

	ioctl(STDIN_FILENO, TIOCGWINSZ, &size);

	tty_len=size.ws_col-22;

	printf("终端长度为： %d\n", tty_len);

	progress_bar=(char*)malloc(tty_len*sizeof(char));

	memset(progress_bar, ' ', tty_len);


	char url[2048] = {0};//ÉèÖÃÄ¬ÈÏµØÖ·Îª±¾»ú,
	char host[2048] = { 0 };//Ô¶³ÌÖ÷»úµØÖ·
	char ip_addr[16] = { 0 };//Ô¶³ÌÖ÷»úIPµØÖ·
	int port = 80;//Ô¶³ÌÖ÷»ú¶Ë¿Ú, httpÄ¬ÈÏ80¶Ë¿Ú
	char file_name[256] = { 0 };

	if (argc == 1){
		printf("No URL to download!\n");
		exit(0);
	}

	int file_name_flag = 0;
	char file_name_tmp[256] = { 0 };
	int opt;
	int longindex = 0;

	optind = 0;

	while ((opt = getopt_long(argc, argv, "t:u:f:h", longopts, &longindex)) != EOF)
		switch (opt){
		case 't':
			thread_flag = atoi(optarg);
			if (thread_flag > 16)
				thread_flag = 16;
			break;
		case 'h':
			diect();
			break;
		case'u':
			strcpy(url, optarg);
			//if (strlen(url))
			break;
		case 'f':
			file_name_flag = 1;
			strcpy(file_name_tmp, optarg);
			break;
	}

	printf("NO.1 Analysis you URL\n");


	resolve_url(url, host, &port, file_name);

	if (file_name_flag != 0){
		printf("You Choose To Use Your Own File Name ,It is : %s \n", file_name_tmp);
		strcpy(file_name, file_name_tmp);
	}

	//printf("NO.2 receive ip address of the file!\n");

	get_ip(host, ip_addr);//µ÷ÓÃº¯ÊýÍ¬·ÃÎÊDNS·þÎñÆ÷»ñÈ¡Ô¶³ÌÖ÷»úµÄIP
	if (strlen(ip_addr) == 0)
	{
		printf("ERROR! Download adress wrong\n");
		return 0;
	}

	printf("Your download message are:\n");
	printf("URL: %s\n", url);
	printf("HOST: %s\n", host);
	printf("IP Address: %s\n", ip_addr);
	printf("Port: %d\n", port);
	printf("Filename : %s\n\n", file_name);

	long file_now_flag = 0;
	int sock = Socket_connect(url, host, ip_addr, port, file_now_flag,file_now_flag);


	char *response = find_end(sock);

	struct header_inf header_inf = get_header_inf(response);

	printf("Header receive!\nThe Response code is %d \n",header_inf.reponse_inf);

	if (check_server(header_inf)!=1){
		printf("Can not download file!\n");
		return 0;
	}
	
	printf("Content-type is : %s\n", header_inf.content_type);
	printf("Content-length is %ld bytes\n\n", (long)header_inf.content_length);

	printf("NOW Downloading...\n");

	if (thread_flag != 0){//多线程处理
		if (access(file_name, 0) == 0){
			if (get_file_size(file_name) == header_inf.content_length){
				printf("File exist!\n");
				return 0;
			}
			else
				remove(file_name);
		}
		if (access("./tmp", 0) < 0){
			mkdir("./tmp", S_IRWXU);
		}
		int bar_len = 0;
		bar_len = tty_len / thread_flag;

		clock_t thread_start = clock();

		omp_set_num_threads(thread_flag);
		//openmp:
		#pragma omp parallel for
		for (int i = 1; i < thread_flag+1; ++i){
			char file_tmp[8] = { 0 };
			strcpy(file_tmp, "./tmp/");
			sprintf(file_tmp+strlen(file_tmp), "%d", i);
			if (access(file_tmp, 0)==0)//如果有遗留的同名缓存则删掉，所以多线程就不支持续传了
				remove(file_tmp);
			//printf("\n缓存文件名为：　%s \n", file_tmp);
			download_file_section(sock, file_tmp, url, host, ip_addr, port, (long)header_inf.content_length*(i - 1) / thread_flag, (long)header_inf.content_length*i / thread_flag - 1,i,bar_len);
			//thread t(download_file_section, sock, file_tmp, url, host, ip_addr, port, (long)header_inf.content_length*(i - 1) / thread_flag, (long)header_inf.content_length*i / thread_flag - 1);
			//t.join();
		}

		merge_tmp("./tmp", file_name, thread_flag);

		clock_t thread_end = clock();

		float thread_time = (float)(thread_end - thread_start) / CLOCKS_PER_SEC;

		float thread_down_length_kb = (float)header_inf.content_length / 1024.00;
		//float thread_used_time_sec = (float)thread_time / 1000000.00;

		printf("\nUse %.2f seconds to download.The average speed is: %.2f MB/s \n", thread_time, thread_down_length_kb / thread_time / 1024.00);
	}
	else{

		if (access(file_name, 0) == 0){
			long file_now = get_file_size(file_name);
			if (file_now == header_inf.content_length){
				printf("File exist!\n");
				return 0;
			}
			else{
				printf("Continue Download!\n");
				printf(">>>>>>>>>>>>>>><<<<<<<<<<<<<<<\n");
				long file_end = header_inf.content_length;
				int tmp_num = 0;
				int tmp_bar_len = 0;
				download_file_section(sock, file_name, url, host, ip_addr, port, file_now, file_end, tmp_num, tmp_bar_len);
			}
		}
		else{
			printf(">>>>>>>>>>>>>>><<<<<<<<<<<<<<<\n");
			download_file(sock, file_name, header_inf.content_length);
		}

		printf("FINAL: Socket Close\n");


	}
	//printf("file large: %d \n",get_file_size(file_name));
	if (header_inf.content_length == get_file_size(file_name))
		printf("\nFile: %s Download success!\n\n", file_name);
	else
	{
		remove(file_name);
		printf("\nError file has been removed,please retry\n\n");
	}
	shutdown(sock, 2);//¹Ø±ÕÌ×½Ó×ÖµÄ½ÓÊÕºÍ·¢ËÍ
	free(progress_bar);
	free(response);
	response = NULL;
	return 0;
}
