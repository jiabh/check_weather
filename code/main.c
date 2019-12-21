#define CITY_NUM 638
#define REQ_LENGTH 84
#define BUFF_SIZE 1024 // buffer size
#define DAY_NEEDED 7 // ��Ҫ��ȡ������
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


CITY** read_dict(); // ��ȡ�����ֵ䣬ʧ�ܷ���NULL
int cmp(const void* a, const void* b);

char* find_city_code(CITY** city_dict, char city_name[]); // ���ֲ��ң����س��д����ַ����������ڷ���NULL
char* get_html(char city_code[]); // ����html������ʧ�ܷ���NULL
DAY_TEMPERATURE* get_temperature(char html[]); // ����DAY�����׵�ַ

// output functions
void output(char city_name[], DAY_TEMPERATURE temperature_array[]); // �������
int get_february_day_tot(int year); // ������ݣ����ش���2�µ�����
void to_tomorrow(struct tm* today); // ��today����һ��
void tm_to_str(struct tm* today, char tm_str[6]); // ��todayת��Ϊ�ַ��������tm_str

int main()
{
	CITY** city_dict; // �����ֵ�
	char* html;
	char input[21];
	char* city_code;
	DAY_TEMPERATURE* temperature_array;
	int flag;
	int i;

	city_dict = read_dict();
	if (city_dict == NULL) {
		printf("��ȡdict.txtʧ�ܣ�\n");
		return -1;
	}
	
	while (1) // ��ѭ��
	{
		printf("������Ҫ��ѯ�ĳ�����(����0�˳�): ");
		flag = scanf("%20s", input);

		if (flag == 1) // ����Ϸ�
		{
			if (input[0] == '0' && input[1] == '\0') // �û����˳�
				break;

			city_code = find_city_code(city_dict, input); // ���ҳ��д���

			if (city_code != NULL)
			{
				printf("��ѯ��...");
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
						printf("������ȡ�쳣��\n");
				}
				else
					printf("���������쳣��\n");
			}
			else
				printf("δ�ҵ��˳��С�\n");
		}
		else
		{
			printf("�������\n");
			while (getchar() != '\n'); // ��ջ�����
		}

		printf("\n");
	}

	// �ͷ�city_dict
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

	printf("���ڶ�ȡdict.txt...");

	err = fopen_s(&fp, "dict.txt", "r"); // ���ֵ�

	if (err) { // ���ֵ����
		return NULL;
	}

	city_dict = calloc(CITY_NUM, sizeof(CITY*)); // �����ڴ棬��ų����ֵ�

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

	qsort(city_dict, CITY_NUM, sizeof(CITY*), cmp); // ���򣬱��ڶ��ֲ���

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

	if (strcmp("��", city_name + strlen(city_name) - 2) == 0)
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
			return city_dict[mid]->code; // ���س��д���
	}

	return NULL; // ���в����ڣ�����NULL
}

char* get_html(char city_code[])
{
	WSADATA data;
	struct hostent* h; // ������
	struct in_addr ina;
	struct sockaddr_in server_addr;
	SOCKET sock; // __int64����

	char req[REQ_LENGTH] = "GET ";
	char buff[BUFF_SIZE];

	int i;
	int err;
	char* ipstr;

	char* html, * html_pre;

	char host[] = "m.weather.com.cn";
	char get[] = "/mweather/         .shtml";
	memmove(get + 10, city_code, 9 * sizeof(char)); // �ϳ�get�ַ���

	err = WSAStartup(MAKEWORD(2, 2), &data);
	if (err)
		return NULL;

	h = gethostbyname(host); // ��ȡ������
	if (h == NULL || h->h_addrtype != AF_INET)
		return NULL;

	// ����IP
	memmove(&ina, h->h_addr, 4);
	ipstr = inet_ntoa(ina); // �������ַת�����ַ�����ʽ

	// Socket��װ
	server_addr.sin_family = AF_INET; // ����ipv4
	server_addr.sin_port = htons(80); // ���ö˿ڣ����˿�ת�������ֽ���
	server_addr.sin_addr.s_addr = inet_addr(ipstr);
	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	connect(sock, (SOCKADDR*)&server_addr, sizeof(server_addr));

	if (sock == -1 || sock == -2)
		return NULL;

	// ��������
	strcat(req, get);
	strcat(req, " HTTP/1.1\r\nHost:");
	strcat(req, host);
	strcat(req, "\r\nConnection:Close\r\n\r\n");

	err = send(sock, req, strlen(req), 0);
	if (err == SOCKET_ERROR) // send����
	{
		closesocket(sock);
		WSACleanup();
		return NULL;
	}

	// ��ȡ��ҳ����
	html = calloc(BUFF_SIZE, sizeof(char));
	if (html == NULL) // ����ʧ��
	{
		closesocket(sock);
		WSACleanup();
		return NULL;
	}
	memset(buff, 0, sizeof(buff));

	err = recv(sock, buff, sizeof(buff) - sizeof(char), 0);
	for (i = 1; err > 0; i++)
	{
		html_pre = html; // ����������Ϊ��realloc�쳣ʱ�ܹ��ͷ�html
		html = realloc(html, i * sizeof(buff));
		if (html == NULL)
		{
			closesocket(sock);
			free(html_pre); // ��ԭ����free��
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
	int pday; // temperature_arrayָ��
	int p_temperature; // �¶��ַ���ָ��

	char highest_pattern[] = "d-temp\">"; // �����pattern
	char lowest_pattern[] = "n-temp\">"; // �����pattern
	char end_pattern[] = "°"; // utf-8�����"��"���¶��ַ�����ȡ��ֹ����

	int html_len = strlen(html);
	int pattern_len = strlen(highest_pattern);

	int i, p, t;

	temperature_array = (DAY_TEMPERATURE*)calloc(DAY_NEEDED, sizeof(DAY_TEMPERATURE*));

	pday = -1;
	for (i = 0; i < html_len - pattern_len + 1; i++) // ��һ�֣�ƥ�������
	{
		for (t = i, p = 0; p < pattern_len; t++, p++) { // ����ƥ�������
			if (html[t] != highest_pattern[p])
				break;
		}

		if (p == pattern_len) // ƥ�䵽����£���ȡ���������
		{
			if (pday >= 0)
			{
				for (p_temperature = 0; html[t] != end_pattern[0]; t++, p_temperature++)
					temperature_array[pday].highest[p_temperature] = html[t];

				temperature_array[pday].highest[p_temperature] = '\0'; // ����ַ�����β
				pday++;
			}
			else // pday < 0�����������£�����
				pday++;
			
			if (pday == DAY_NEEDED) // �ѻ����Ҫ���������ݣ�ֹͣ
				break;
		}
	}
	if (pday != DAY_NEEDED) // ��ȡ�����쳣
	{
		free(temperature_array);
		return NULL;
	}

	pday = -1;
	for (i = 0; i < html_len - pattern_len + 1; i++) // �ڶ��֣�ƥ�������
	{
		for (t = i, p = 0; p < pattern_len; t++, p++) { // ����ƥ�������
			if (html[t] != lowest_pattern[p])
				break;
		}

		if (p == pattern_len) // ƥ�䵽����£���ȡ���������
		{
			if (pday >= 0)
			{
				for (p_temperature = 0; html[t] != end_pattern[0]; t++, p_temperature++)
					temperature_array[pday].lowest[p_temperature] = html[t];

				temperature_array[pday].lowest[p_temperature] = '\0'; // ����ַ�����β
				pday++;
			}
			else // pday < 0�����������£�����
				pday++;
			
			if (pday == DAY_NEEDED) // �ѻ����Ҫ���������ݣ�ֹͣ
				break;
		}
	}
	if (pday != DAY_NEEDED) // ��ȡ�����쳣
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
	
	printf("%s�н�%d����������:\n\n", city_name, DAY_NEEDED);

	// ���header
	printf("           ����   ����");
	to_tomorrow(local);
	for (i = 2; i < DAY_NEEDED; i++)
	{
		to_tomorrow(local);
		tm_to_str(local, tm_str);
		printf("%7s", tm_str);
	}
	printf("\n");

	// ��������
	printf("�����: ");
	for (i = 0; i < DAY_NEEDED; i++)
	{
		printf("%4s��C", temperature_array[i].highest);
	}
	printf("\n");

	// ��������
	printf("�����: ");
	for (i = 0; i < DAY_NEEDED; i++)
	{
		printf("%4s��C", temperature_array[i].lowest);
	}
	printf("\n\n");
}

int get_february_day_tot(int year) // ������ݣ����ش���2�µ�����
{
	if ((year % 4 == 0) && (year % 100 != 0) || (year % 400 == 0))
		return 29;
	else
		return 28;
}

void to_tomorrow(struct tm* today)
{
	//                     1  2  3  4  5  6  7  8  9 10 11 12 ��
	int flag_array[12] = { 1, 3, 1, 2, 1, 2, 1, 1, 2, 1, 2, 1 };
	int day_tot;

	if (today->tm_mon == 12 && today->tm_mday == 31) // �������
	{
		today->tm_year++;
		today->tm_mon = 1;
		today->tm_mday = 1;
	}
	else // û�����
		switch (flag_array[today->tm_mon])
		{
			case 1: // һ������31��
				if (today->tm_mday == 31)
				{
					today->tm_mon++;
					today->tm_mday = 1;
				}
				else
					today->tm_mday++;
				break;

			case 2: // һ������30��
				if (today->tm_mday == 30)
				{
					today->tm_mon++;
					today->tm_mday = 1;
				}
				else
					today->tm_mday++;
				break;

			case 3: // 2��
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
