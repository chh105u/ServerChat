#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <netinet/in.h>

int sockfd=0;
void *ThreadSend(void*);
void *ThreadRecv(void*);
void *downfileRecv(void*);
void *downfileSend(void*);

int main(int argc , char *argv[])
{
	if(argc!=2)
	{
		printf("Please input your name.\n");
		exit(EXIT_FAILURE);
	}
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	if(sockfd==-1)
	{
		perror("Fail to create a socket");
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in info;
	bzero(&info,sizeof(info));
	info.sin_family=PF_INET;

	info.sin_addr.s_addr=inet_addr("127.0.0.1");
	info.sin_port=htons(8700);

	int err=connect(sockfd,(struct sockaddr*)&info,sizeof(info));
	if(err==-1)
	{
		perror("Connection error");
		exit(EXIT_FAILURE);
	}
	
	char receiveMessage[2048]={};
	send(sockfd,argv[1],sizeof(argv[1]),0);
	recv(sockfd,receiveMessage,sizeof(receiveMessage),0);
	printf("%s",receiveMessage);
	
	pthread_t tid1,tid2;
	pthread_create(&tid1,NULL,ThreadSend,NULL);
	pthread_create(&tid2,NULL,ThreadRecv,NULL);
	
	pthread_join(tid2,NULL);
	printf("Close socket\n");
	close(sockfd);
	return 0;
}

void *ThreadSend(void *err)
{
	char message[512]={};
	char username[20]={};
	char arg[5];
	while(1)
	{
		fgets(arg,5,stdin);
		arg[strlen(arg)-1]='\0';
		send(sockfd,arg,sizeof(arg),0);
		if(strcmp(arg,"c")==0)
		{
			fgets(username,20,stdin);
			username[strlen(username)-1]='\0';
			send(sockfd,username,sizeof(username),0);
			fgets(message,512,stdin);
			message[strlen(message)-1]='\0';
			send(sockfd,message,sizeof(message),0);
		}
		else if(strcmp(arg,"s")==0)
		{
			fgets(message,512,stdin);
			message[strlen(message)-1]='\0';
			send(sockfd,message,sizeof(message),0);
		}
		else if(strcmp(arg,"t")==0)
		{
			fgets(username,20,stdin);
			username[strlen(username)-1]='\0';
			send(sockfd,username,sizeof(username),0);
			fgets(message,512,stdin);
			message[strlen(message)-1]='\0';
			FILE *fp=fopen(message,"rb");
			if(fp==NULL)
			{
				char err[5]={"err"};
				send(sockfd,err,sizeof(err),0);
				continue;
			}
			else
			{
				send(sockfd,message,sizeof(username),0);
				printf("已傳送檔案要求，等待對方接收。\n");
//				pthread_t tid;
//				pthread_create(&tid,NULL,downfileSend,fp);
				fclose(fp);
			}
		}
//		else if(strcmp(arg,"e")==0)
//		{
//			close(sockfd);
//		}
	}
}

void *ThreadRecv(void *arg)
{
	int err;
	char message[512];
	while(1)
	{
		err=recv(sockfd,message,sizeof(message),0);
		if(err==-1)
		{
			sleep(500);
			continue;
		}
//		if(strcmp(message,"正在傳輸中...")==0)
//		{
//			FILE *fp=fopen("downfile","wb");
//			char buffer[1024];
//			int nCount;
//			while((nCount=recv(sockfd,buffer,1024,0)))
//				fwrite(buffer,nCount,1,fp);
//			fclose(fp);
//			pthread_t tid;
//			pthread_create(&tid,NULL,downfileRecv,fp);
//		}
		if(strlen(message)!=0)
			printf("%s\n",message);
		else
			sleep(100);
	}
}
void *downfileRecv(void *arg)
{
	FILE *fp=(FILE*)arg;
	char buffer[1024];
	int nCount;
	while(1)
	{
		nCount=recv(sockfd,buffer,1024,0);
		fwrite(buffer,nCount,1,fp);
		if(feof(fp))
			break;
	}
	printf("fuck?");
	fclose(fp);
}
void *downfileSend(void *arg)
{
	FILE *fp=(FILE*)arg;
	char buffer[1024];
	int nCount;
	while(1)
	{
		nCount=fread(buffer,1024,1,fp);
		send(sockfd,buffer,nCount,0);
		if(feof(fp))
			break;
	}
	fclose(fp);
}
