#define CITY_NUM 638
#define REQ_LENGTH 84
#define BUFF_SIZE 1024 // buffer size
#define DAY_NEEDED 7 // 需要提取的天数
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")

typedef struct city {
	char* city_name;
	char* code;
} CITY;

typedef struct day_temperature {
	char highest[4];
	char lowest[4];
} DAY_TEMPERATURE;


CITY** read_dict(); // 读取城市字典，失败返回NULL
int cmp(const void* a, const void* b);

char* find_city_code(CITY** city_dict, char city_name[]); // 二分查找，返回城市代码字符串，不存在返回NULL
char* get_html(char city_code[]); // 返回html，请求失败返回NULL
DAY_TEMPERATURE* get_temperature(char html[]); // 返回DAY数组首地址

// output functions
void output(char city_name[], DAY_TEMPERATURE temperature_array[]); // 输出气温
int get_february_day_tot(int year); // 输入年份，返回此年2月的天数
void to_tomorrow(struct tm* today); // 将today增加一天
void tm_to_str(struct tm* today, char tm_str[6]); // 将today转换为字符串后存至tm_str

int main()
{
	CITY** city_dict; // 城市字典
	char* html;
	char input[21];
	char* city_code;
	DAY_TEMPERATURE* temperature_array;
	int flag;
	int i;

	city_dict = read_dict();
	if (city_dict == NULL) {
		printf("读取dict.txt失败！\n");
		return -1;
	}
	
	while (1) // 主循环
	{
		printf("请输入要查询的城市名(输入0退出): ");
		flag = scanf("%20s", input);

		if (flag == 1) // 输入合法
		{
			if (input[0] == '0' && input[1] == '\0') // 用户想退出
				break;

			city_code = find_city_code(city_dict, input); // 查找城市代码

			if (city_code != NULL)
			{
				printf("查询中...");
				html = get_html(city_code);
				printf("\r");

				if (html != NULL)
				{
					temperature_array = get_temperature(html);
					free(html);

					if (temperature_array != NULL)
					{
						output(input, temperature_array);
						free(temperature_array);
					}
					else
						printf("数据提取异常！\n");
				}
				else
					printf("网络连接异常！\n");
			}
			else
				printf("未找到此城市。\n");
		}
		else
		{
			printf("输入错误！\n");
			while (getchar() != '\n'); // 清空缓冲区
		}

		printf("\n");
	}

	// 释放city_dict
	for (i = 0; i < CITY_NUM; i++)
		free(city_dict[i]);
	free(city_dict);

	return 0;
}


CITY** read_dict()
{
	FILE* fp;
	errno_t err;

	CITY** city_dict;

	char buff[100];
	CITY* city_t;
	char* t;
	int i;

	printf("正在读取dict.txt...");

	err = fopen_s(&fp, "dict.txt", "r"); // 打开字典

	if (err) { // 打开字典出错
		return NULL;
	}

	city_dict = calloc(CITY_NUM, sizeof(CITY*)); // 申请内存，存放城市字典

	for (i = 0; i < CITY_NUM; i++)
	{
		city_t = malloc(sizeof(CITY));
		city_dict[i] = city_t;

		fscanf(fp, "%s", buff);
		t = calloc(strlen(buff) + 1, sizeof(char));
		strcpy(t, buff);
		city_t->city_name = t;

		fscanf(fp, "%s", buff);
		t = calloc(strlen(buff) + 1, sizeof(char));
		strcpy(t, buff);
		city_t->code = t;
	}

	fclose(fp);
	printf("\r");

	qsort(city_dict, CITY_NUM, sizeof(CITY*), cmp); // 排序，便于二分查找

	return city_dict;
}

int cmp(const void* a, const void* b)
{
	return strcmp((*(CITY**)a)->city_name, (*(CITY**)b)->city_name);
}


char* find_city_code(CITY** city_dict, char city_name[])
{
	int i, mid, j, flag;

	i = 0;
	j = CITY_NUM;

	if (strcmp("市", city_name + strlen(city_name) - 2) == 0)
		city_name[strlen(city_name) - 2] = '\0';

	while (i <= j)
	{
		mid = (i + j) / 2;
		flag = strcmp(city_name, city_dict[mid]->city_name);

		if (flag > 0)
			i = mid + 1;
		else if (flag < 0)
			j = mid - 1;
		else
			return city_dict[mid]->code; // 返回城市代码
	}

	return NULL; // 城市不存在，返回NULL
}

char* get_html(char city_code[])
{
	WSADATA data;
	struct hostent* h; // 主机名
	struct in_addr ina;
	struct sockaddr_in server_addr;
	SOCKET sock; // __int64类型

	char req[REQ_LENGTH] = "GET ";
	char buff[BUFF_SIZE];

	int i;
	int err;
	char* ipstr;

	char* html, * html_pre;

	char host[] = "m.weather.com.cn";
	char get[] = "/mweather/         .shtml";
	memmove(get + 10, city_code, 9 * sizeof(char)); // 合成get字符串

	err = WSAStartup(MAKEWORD(2, 2), &data);
	if (err)
		return NULL;

	h = gethostbyname(host); // 获取主机名
	if (h == NULL || h->h_addrtype != AF_INET)
		return NULL;

	// 解析IP
	memmove(&ina, h->h_addr, 4);
	ipstr = inet_ntoa(ina); // 将网络地址转换成字符串格式

	// Socket封装
	server_addr.sin_family = AF_INET; // 设置ipv4
	server_addr.sin_port = htons(80); // 设置端口，将端口转成网络字节序
	server_addr.sin_addr.s_addr = inet_addr(ipstr);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr));

	if (sock == -1 || sock == -2)
		return NULL;

	// 发送请求
	strcat(req, get);
	strcat(req, " HTTP/1.1\r\nHost:");
	strcat(req, host);
	strcat(req, "\r\nConnection:Close\r\n\r\n");

	err = send(sock, req, strlen(req), 0);
	if (err == SOCKET_ERROR) // send错误
	{
		closesocket(sock);
		WSACleanup();
		return NULL;
	}

	// 获取网页内容
	html = calloc(BUFF_SIZE, sizeof(char));
	if (html == NULL) // 申请失败
	{
		closesocket(sock);
		WSACleanup();
		return NULL;
	}
	memset(buff, 0, sizeof(buff));

	err = recv(sock, buff, sizeof(buff) - sizeof(char), 0);
	for (i = 1; err > 0; i++)
	{
		html_pre = html; // 创建副本，为了realloc异常时能够释放html
		html = realloc(html, i * sizeof(buff));
		if (html == NULL)
		{
			closesocket(sock);
			free(html_pre); // 把原来的free掉
			WSACleanup();
			return NULL;
		}
		strcat(html, buff);
		memset(buff, 0, sizeof(buff));
		err = recv(sock, buff, sizeof(buff) - sizeof(char), 0);
	}

	closesocket(sock);
	WSACleanup();
	return html;
}

DAY_TEMPERATURE* get_temperature(char html[])
{
	DAY_TEMPERATURE* temperature_array;
	int pday; // temperature_array指针
	int p_temperature; // 温度字符串指针

	char highest_pattern[] = "d-temp\">"; // 最高温pattern
	char lowest_pattern[] = "n-temp\">"; // 最低温pattern
	char end_pattern[] = "掳"; // utf-8编码的"°"，温度字符串提取终止符号

	int html_len = strlen(html);
	int pattern_len = strlen(highest_pattern);

	int i, p, t;

	temperature_array = (DAY_TEMPERATURE*)calloc(DAY_NEEDED, sizeof(DAY_TEMPERATURE*));

	pday = -1;
	for (i = 0; i < html_len - pattern_len + 1; i++) // 第一轮，匹配最高温
	{
		for (t = i, p = 0; p < pattern_len; t++, p++) { // 尝试匹配最高温
			if (html[t] != highest_pattern[p])
				break;
		}

		if (p == pattern_len) // 匹配到最高温，提取最高温数据
		{
			if (pday >= 0)
			{
				for (p_temperature = 0; html[t] != end_pattern[0]; t++, p_temperature++)
					temperature_array[pday].highest[p_temperature] = html[t];

				temperature_array[pday].highest[p_temperature] = '\0'; // 添加字符串结尾
				pday++;
			}
			else // pday < 0，是昨日气温，跳过
				pday++;
			
			if (pday == DAY_NEEDED) // 已获得需要天数的数据，停止
				break;
		}
	}
	if (pday != DAY_NEEDED) // 获取数据异常
	{
		free(temperature_array);
		return NULL;
	}

	pday = -1;
	for (i = 0; i < html_len - pattern_len + 1; i++) // 第二轮，匹配最低温
	{
		for (t = i, p = 0; p < pattern_len; t++, p++) { // 尝试匹配最低温
			if (html[t] != lowest_pattern[p])
				break;
		}

		if (p == pattern_len) // 匹配到最低温，提取最低温数据
		{
			if (pday >= 0)
			{
				for (p_temperature = 0; html[t] != end_pattern[0]; t++, p_temperature++)
					temperature_array[pday].lowest[p_temperature] = html[t];

				temperature_array[pday].lowest[p_temperature] = '\0'; // 添加字符串结尾
				pday++;
			}
			else // pday < 0，是昨日气温，跳过
				pday++;
			
			if (pday == DAY_NEEDED) // 已获得需要天数的数据，停止
				break;
		}
	}
	if (pday != DAY_NEEDED) // 获取数据异常
	{
		free(temperature_array);
		return NULL;
	}

	return temperature_array;
}

void output(char city_name[], DAY_TEMPERATURE temperature_array[])
{
	int i;

	struct tm* local;
	time_t t;
	char tm_str[6];

	t = time(NULL);
	local = localtime(&t);
	
	printf("%s市近%d日气温如下:\n\n", city_name, DAY_NEEDED);

	// 输出header
	printf("           今天   明天");
	to_tomorrow(local);
	for (i = 2; i < DAY_NEEDED; i++)
	{
		to_tomorrow(local);
		tm_to_str(local, tm_str);
		printf("%7s", tm_str);
	}
	printf("\n");

	// 输出最高温
	printf("最高温: ");
	for (i = 0; i < DAY_NEEDED; i++)
	{
		printf("%4s°C", temperature_array[i].highest);
	}
	printf("\n");

	// 输出最低温
	printf("最低温: ");
	for (i = 0; i < DAY_NEEDED; i++)
	{
		printf("%4s°C", temperature_array[i].lowest);
	}
	printf("\n\n");
}

int get_february_day_tot(int year) // 输入年份，返回此年2月的天数
{
	if ((year % 4 == 0) && (year % 100 != 0) || (year % 400 == 0))
		return 29;
	else
		return 28;
}

void to_tomorrow(struct tm* today)
{
	//                     1  2  3  4  5  6  7  8  9 10 11 12 月
	int flag_array[12] = { 1, 3, 1, 2, 1, 2, 1, 1, 2, 1, 2, 1 };
	int day_tot;

	if (today->tm_mon == 12 && today->tm_mday == 31) // 到年底了
	{
		today->tm_year++;
		today->tm_mon = 1;
		today->tm_mday = 1;
	}
	else // 没到年底
		switch (flag_array[today->tm_mon])
		{
			case 1: // 一个月有31天
				if (today->tm_mday == 31)
				{
					today->tm_mon++;
					today->tm_mday = 1;
				}
				else
					today->tm_mday++;
				break;

			case 2: // 一个月有30天
				if (today->tm_mday == 30)
				{
					today->tm_mon++;
					today->tm_mday = 1;
				}
				else
					today->tm_mday++;
				break;

			case 3: // 2月
				day_tot = get_february_day_tot(today->tm_year);
				if (today->tm_mday == day_tot)
				{
					today->tm_mon++;
					today->tm_mday = 1;
				}
				else
					today->tm_mday++;
				break;
		}
}

void tm_to_str(struct tm* today, char tm_str[6])
{
	int month;
	int day;

	month = today->tm_mon + 1;
	day = today->tm_mday;

	tm_str[0] = '0' + month / 10;
	tm_str[1] = '0' + month % 10;
	tm_str[2] = '-';
	tm_str[3] = '0' + day / 10;
	tm_str[4] = '0' + day % 10;
	tm_str[5] = '\0';
}
