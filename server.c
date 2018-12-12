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

void CreateNewPlayer(int sockfd);
void *ThreadAccept(void*);
void *ThreadSend(void*);
void *ThreadRecv(void*);
void *ThreadUser(void*);
void *ThreadSendPri(void*);
void *ThreadTrans(void*);
void *ThreadTransY(void*);
void *ThreadRes(void*);
void *downfileRecv(void*);
void *downfileSend(void*);
void *client(void *);
FILE *fp;
static int ClientCount=0;

struct LinkListClient
{
	int fd;
	pthread_t tid;
	char name[20];
	char privatename[20];
	char message[512];
	char filename[512];
	struct LinkListClient *next;
};
struct LinkListClient *node;
struct LinkListClient *insert(struct LinkListClient *list,int);

int main(int argc , char *argv[])
{
	int ServerSockfd=0,ClientSockfd=0;
	struct sockaddr_in ServerInfo,ClientInfo;
	int ClientAddrlen=sizeof(ClientInfo);
	bzero(&ServerInfo,sizeof(ServerInfo));

	ServerInfo.sin_family=PF_INET;
	ServerInfo.sin_addr.s_addr=INADDR_ANY;
	ServerInfo.sin_port=htons(8700);
	
	ServerSockfd=socket(AF_INET,SOCK_STREAM,0);
	if(ServerSockfd==-1)
	{
		perror("Fail to create a sockft.");
		exit(EXIT_FAILURE);
	}
	printf("Server is start.\n");
	
	bind(ServerSockfd,(struct sockaddr*)&ServerInfo,sizeof(ServerInfo));

	listen(ServerSockfd,5);

	while(1)
	{
		printf("server waiting\n");
		ClientSockfd=accept(ServerSockfd,(struct sockaddr*)&ClientInfo,&ClientAddrlen);
		++ClientCount;
		printf("Now client count:%d\n",ClientCount);
		CreateNewPlayer(ClientSockfd);
	}

	close(ServerSockfd);
	return 0;
}

void *ThreadRecv(void* err)
{
	char buffer[1024];
	int nCount;
	char filename[]={"tmp"};
//	FILE *fp;
	char receiveMessage[512]={};
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	while(1)
	{	
		recv(thisfd->fd,receiveMessage,sizeof(receiveMessage),0);
		if(strcmp("s",receiveMessage)==0)
		{
			recv(thisfd->fd,receiveMessage,sizeof(receiveMessage),0);
			char message[512]={};
			strcat(message,thisfd->name);
			strcat(message,"：");
			strcat(message,receiveMessage);
			strcpy(thisfd->message,message);
			pthread_t tid;
			pthread_create(&tid,NULL,ThreadSend,thisfd);
		}
		else if(strcmp("f",receiveMessage)==0)
		{
			pthread_t tid;
			pthread_create(&tid,NULL,ThreadUser,thisfd);
		}
		else if(strcmp("c",receiveMessage)==0)
		{
			char username[20]={};
			char message[512]={};
			recv(thisfd->fd,username,sizeof(username),0);
			strcpy(thisfd->privatename,username);
		
			recv(thisfd->fd,receiveMessage,sizeof(receiveMessage),0);
			strcat(message,thisfd->name);
			strcat(message,"向你傳送私人訊息：");
			strcat(message,receiveMessage);
			strcpy(thisfd->message,message);
			pthread_t tid;
			pthread_create(&tid,NULL,ThreadSendPri,thisfd);
		}
		else if(strcmp("t",receiveMessage)==0)
		{
			char username[20]={};
			char message[512]={};
			recv(thisfd->fd,username,sizeof(username),0);
			strcpy(thisfd->privatename,username);
			
			recv(thisfd->fd,receiveMessage,sizeof(receiveMessage),0);
			if(strcmp(receiveMessage,"err")==0)
			{
				char error[512]={"查無此檔案"};
				strcpy(thisfd->privatename,"");
				send(thisfd->fd,error,sizeof(error),0);
				continue;
			}
			else
			{
				strcat(message,thisfd->name);
				strcat(message,"向你傳送一個檔案為：");
				strcat(message,receiveMessage);
				strcat(message,"，請問是否接收？y/n");
				strcpy(thisfd->message,message);
				pthread_t tid;
				pthread_create(&tid,NULL,ThreadTrans,thisfd);
//				fp=fopen(filename,"wb");
//				while((nCount=recv(thisfd->fd,buffer,1024,0))>0)
//					fwrite(buffer,nCount,1,fp);
//				pthread_t tid2;
//				pthread_create(&tid2,NULL,downfileRecv,thisfd);
			}
			
		}
		else if(strcmp("y",receiveMessage)==0)
		{
			if(strcmp(thisfd->privatename,"")==0)
			{
				char message[512]={"未有檔案傳輸要求"};
				send(thisfd->fd,message,sizeof(message),0);
				continue;
			}
			pthread_t tid;
			pthread_create(&tid,NULL,ThreadTransY,thisfd);
		}
		else if(strcmp("n",receiveMessage)==0)
		{
			if(strcmp(thisfd->privatename,"")==0)
			{
				char message[512]={"未有檔案傳輸要求"};
				send(thisfd->fd,message,sizeof(message),0);
				continue;
			}

			pthread_t tid;
			pthread_create(&tid,NULL,ThreadRes,thisfd);
		}
/*		else if(strcmp("e",receiveMessage)==0)
		{
			char message[30]={};
			strcat(message,thisfd->name);
			strcat(message,"下線。");
			strcpy(thisfd->message,message);
			pthread_t tid;
			pthread_create(&tid,NULL,ThreadSend,thisfd);
			--ClientCount;
		}*/
		else
		{
			char message[]={"你的指令未查到，請重新輸入一次"};
			send(thisfd->fd,message,sizeof(message),0);
		}
	}
}
void *ThreadSend(void* err)
{
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	struct LinkListClient *ptr=node;
	while(ptr!=NULL)
	{
		send(ptr->fd,thisfd->message,sizeof(thisfd->message),0);
		ptr=ptr->next;
	}
}

void *ThreadUser(void* err)
{
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	struct LinkListClient *ptr=node;
	int i=1;
	char num[5];
	char message[512]={"現在正在使用server的使用者有：\n"};
	while(ptr!=NULL)
	{
		strcat(message,"	");
		sprintf(num,"%d",i);
		strcat(message,num);
		strcat(message,") ");
		strcat(message,ptr->name);
		strcat(message,"\n");
		ptr=ptr->next;
		i++;
	}
	send(thisfd->fd,message,sizeof(message),0);

}

void *ThreadSendPri(void *err)
{
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	struct LinkListClient *ptr=node;
	char errormessage[50]={"使用者未上線或查無此人。"};
	while(ptr!=NULL)
	{
		if(strcmp(thisfd->privatename,ptr->name)==0)
			break;
		ptr=ptr->next;
	}
	if(ptr!=NULL)
	{
		strcpy(thisfd->privatename,"");
		send(ptr->fd,thisfd->message,sizeof(thisfd->message),0);
		char message[512]={"你已向"};
		strcat(message,ptr->name);
		strcat(message,"傳送私人訊息");
		send(thisfd->fd,message,sizeof(message),0);
	}
	else
	{
		strcpy(thisfd->privatename,"");
		send(thisfd->fd,errormessage,sizeof(errormessage),0);
	}
}

void *ThreadTrans(void *err)
{
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	struct LinkListClient *ptr=node;
	char errormessage[50]={"使用者未上線或查無此人。"};
	while(ptr!=NULL)
	{
		if(strcmp(thisfd->privatename,ptr->name)==0)
			break;
		ptr=ptr->next;
	}
	if(ptr!=NULL)
	{
		strcpy(ptr->privatename,thisfd->name);
		send(ptr->fd,thisfd->message,sizeof(thisfd->message),0);
	}
	else
	{
		strcpy(thisfd->privatename,"");
		send(thisfd->fd,errormessage,sizeof(errormessage),0);
	}
}

void *ThreadTransY(void *err)
{
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	struct LinkListClient *ptr=node;
	while(ptr!=NULL)
	{
		if(strcmp(thisfd->privatename,ptr->name)==0)
			break;
		ptr=ptr->next;
	}
	char message[512]={"對方已接受你的傳送要求"};
	send(ptr->fd,message,sizeof(message),0);
	strcpy(message,"你已接受使用者：");
	strcat(message,thisfd->privatename);
	strcat(message," 對你傳送檔案的要求。");
	send(thisfd->fd,message,sizeof(message),0);
	strcpy(message,"正在傳輸中...");
	send(thisfd->fd,message,sizeof(message),0);
	strcpy(ptr->privatename,"");
	strcpy(thisfd->privatename,"");
//	char buffer[1024];
//	int nCount;
//	while((nCount=fread(buffer,1024,1,fp))>0)
//		send(thisfd->fd,buffer,nCount,0);
//	pthread_t tid;
//	pthread_create(&tid,NULL,downfileSend,thisfd);
	strcpy(message,"傳輸完畢。");
	send(thisfd->fd,message,sizeof(message),0);
//	fclose(fp);
}

void *ThreadRes(void *err)
{
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	struct LinkListClient *ptr=node;
	while(ptr!=NULL)
	{
		if(strcmp(thisfd->privatename,ptr->name)==0)
			break;
		ptr=ptr->next;
	}
	char message[512]={"對方已拒絕你的傳送要求"};
	send(ptr->fd,message,sizeof(message),0);
	strcpy(message,"你已拒絕使用者：");
	strcat(message,thisfd->privatename);
	strcat(message," 對你傳送檔案的要求。");
	strcpy(ptr->privatename,"");
	strcpy(thisfd->privatename,"");
	send(thisfd->fd,message,sizeof(message),0);
}

void *downfileRecv(void *err)
{
	char buffer[1024];
	int nCount;
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	while(1)
	{
		nCount=recv(thisfd->fd,buffer,1024,0);
		fwrite(buffer,nCount,1,fp);
		if(feof(fp))
			break;
	}
}

void *downfileSend(void *err)
{
	char buffer[1024];
	int nCount;
	struct LinkListClient *thisfd=(struct LinkListClient*)err;
	while(1)
	{
		nCount=fread(buffer,1024,1,fp);
		send(thisfd->fd,buffer,nCount,0);
		if(feof(fp))
			break;
	}
}

void CreateNewPlayer(int clientsockfd)
{
	node=insert(node,clientsockfd);
	pthread_create(&node->tid,NULL,client,NULL);
}

struct LinkListClient *insert(struct LinkListClient *list,int clientsockfd)
{
	struct LinkListClient *ptr;
	ptr=(struct LinkListClient *)malloc(sizeof(struct LinkListClient));
	ptr->fd=clientsockfd;
	ptr->next=list;
	list=ptr;
	return list;
}

void* client(void* arg)
{
	char ClientName[20];
	struct LinkListClient *thisfd=node;
	recv(thisfd->fd,ClientName,sizeof(ClientName),0);
	strcpy(thisfd->name,ClientName);
	char FirstSendMessage[2048]={"這是一個群組聊天室 server。\n使用者連上server時會送出自己的帳號名稱，你可以使用以下功能：\n	f)查看哪些使用者正在server線上\n	s)送出訊息給所有連到同一server的使用者\n	c)指定傳送訊息給正在線上的某個使用者\n	t)傳送檔案給某一個使用者，對方可決定是否接收\n\n"};
//	char FirstSendMessage[2048]={"這是一個群組聊天室 server。\n使用者連上server時會送出自己的帳號名稱，你可以使用以下功能：\n	f)查看哪些使用者正在server線上\n	s)送出訊息給所有連到同一server的使用者\n	c)指定傳送訊息給正在線上的某個使用者\n	t)傳送檔案給某一個使用者，對方可決定是否接收\n	e)離開server\n\n"};
	strcat(FirstSendMessage,"你的使用者帳號名稱為：");
	strncat(FirstSendMessage,ClientName,sizeof(ClientName));
	strcat(FirstSendMessage,"\n\n");
	
	char receiveMessage[512];
	send(thisfd->fd,FirstSendMessage,sizeof(FirstSendMessage),0);
	
	pthread_t tid;
	pthread_create(&tid,NULL,ThreadRecv,thisfd);
}
