#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <string.h>
#include <fcntl.h>
#include <bits/stdc++.h>
#include <netdb.h>
#include <time.h>
#define PACKET_SIZE 64
#define PAYLOADSIZE PACKET_SIZE - 8
#define PORT 8888
#define RECV_TIMEOUT 2
using namespace std;
bool checkValidIp(string ip)
{
	struct sockaddr_in s;
	if(inet_pton(AF_INET,&ip[0],&s))
	{
		return true;
	}
	return false;
}
struct packetStruct
{
	struct icmphdr h;
	char msg[PAYLOADSIZE];
};
void printError()
{
	cout << "Request timed out or host unreacheable" << endl;
	exit(0);
}
unsigned short checkSum(void *b, int len)
{    unsigned short *buf = (unsigned short *)b;
    unsigned int sum=0;
    unsigned short result;
  
    for ( sum = 0; len > 1; len -= 2 )
        sum += *buf++;
    if ( len == 1 )
        sum += *(unsigned char*)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}
double getTimeInterval(timespec st,timespec ed)
{
	double val = ed.tv_sec-st.tv_sec;
	val *= 1000;
	val += (ed.tv_nsec - st.tv_nsec)/1e6;
	return val;
}
void ping(int fd,struct sockaddr_in *ping_addr,char *ip_addr)
{
	double RTT_CALC = 0;
	struct timespec st,ed;
	struct packetStruct pack;
	struct sockaddr_in ad;
	memset(&pack,0,sizeof(pack));
	pack.h.type=ICMP_ECHO;
	memset(pack.msg,0,sizeof(pack.msg));
	pack.h.checksum = checkSum((void*)&pack,sizeof(pack));
	clock_gettime(CLOCK_MONOTONIC,&st);
	int tmp = sendto(fd,&pack,sizeof(pack),0,(struct sockaddr*) ping_addr,sizeof(*ping_addr));
	if(tmp <= 0)
	{
		printError();
		return;
	}
	int ad_l = sizeof(ad);
	tmp = 0;
	time_t  maxAllowd = time(0) + RECV_TIMEOUT;
	fcntl(STDIN_FILENO,F_SETFL,	fcntl(STDIN_FILENO,F_GETFL,0) | O_NONBLOCK);
	while(time(0) < maxAllowd)
	{
		tmp = recvfrom(fd,&pack,sizeof(pack),0,(struct sockaddr*)&ad,(unsigned int*)&ad_l);
		if(tmp > 0)
		{
			goto done;
		}
		else if(tmp < 0)
		{
			printError();
		}
	}
	printError();
	done:
	clock_gettime(CLOCK_MONOTONIC,&ed);
	RTT_CALC = getTimeInterval(st,ed);
	string tmpIp = string(ip_addr);
	cout << "Reply from " << tmpIp << ". RTT = " << RTT_CALC << " ms" << endl;

}
int main(int argc,char ** argv)
{
	int fd;
	struct in_addr saddr;
	struct sockaddr_in ping_addr;
	char *ip_addr = argv[1];
	if(checkValidIp(ip_addr))
	{
		cout << "Pinging " << ip_addr << endl;
		inet_aton(ip_addr,&saddr);
		ping_addr.sin_port = htons(PORT);
		ping_addr.sin_family = AF_INET;
		ping_addr.sin_addr.s_addr = saddr.s_addr;
		fd = socket(AF_INET,SOCK_RAW,IPPROTO_ICMP);
		if(fd < 0)
		{
			cout << "FAIL" << endl;
			return -1;
		}
		ping(fd,&ping_addr,ip_addr);
		return 0;
	}
	else
	{
		cout << "Bad hostname" << endl;
		return -1;
	}
	return 0;
}