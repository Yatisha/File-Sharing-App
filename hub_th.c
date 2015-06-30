#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>
#include "hashtable.h"

#define PORT 3490
#define BACKLOG 10
#define MAX_CONNECTIONS 10

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

typedef struct client_data
{
    int thread_no;
    int client_fd;
    char *peer_ip;
} client_info;

hashtable_t* hashtable;

void disconnectPeerFromHub(char * peer_ip); // remove the peers ip and its fileList from hub's table 
char * searchHub(char *filename); /*searches the hub's table for a given filename 
								   and returns ip of seeder*/ 

void * doStuffWithClient(void *clientData);
int recieveFileList(int client_fd ,char * peer_ip);
void quit();

int main()
{   
    signal(SIGINT,quit);
    pthread_t tid[MAX_CONNECTIONS]; //create threads for handling connections from peer
    client_info clientInf[MAX_CONNECTIONS]; //store data of each connection to client

    hashtable = ht_create( 50 );
    struct sockaddr_in hub;
    struct sockaddr_in client;
    int status,socket_fd, client_fd,num;
    socklen_t size;

    int yes =1;

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        fprintf(stderr, "Socket failure!!\n");
        exit(1);
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }

    memset(&hub, 0, sizeof(hub));
    memset(&client,0,sizeof(client));

    hub.sin_family = AF_INET;
    hub.sin_port = htons(PORT);
    hub.sin_addr.s_addr = INADDR_ANY;

    if ((bind(socket_fd, (struct sockaddr *)&hub, sizeof(struct sockaddr ))) == -1)    { 
        exit(1);
    }

    if ((listen(socket_fd, BACKLOG))== -1){
        fprintf(stderr, "Listening Failure\n");
        exit(1);
    }

    int connections = 0;
    while(1)
    {
        size = sizeof(struct sockaddr_in);
        printf("\nHashtable before connection Number %d",connections );  
        print_hashtable(hashtable);

        if ((client_fd = accept(socket_fd, (struct sockaddr *)&client, &size))==-1) {
            
            perror("Accept error");
            continue;
        }

        char *peer_ip = inet_ntoa(client.sin_addr); //Store this ip in some data structure
        printf("HUB got connection from client %s\n", peer_ip);
        

        (clientInf[connections]).thread_no = connections;
        (clientInf[connections]).client_fd = client_fd;
        (clientInf[connections]).peer_ip = peer_ip;
        
        pthread_attr_t attr;
        int s;

        s = pthread_attr_init(&attr);
        if (s != 0)
            printf("\npthread_attr_init");

        s = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (s != 0)
            printf("\npthread_attr_setdetachstate");

        s = pthread_create(&(tid[connections]), &attr, &doStuffWithClient, &(clientInf[connections]));
        if (s != 0)
            printf("\npthread_create");

        s = pthread_attr_destroy(&attr); /* No longer needed */
        if (s != 0)
            printf("\npthread_attr_destroy");

        pthread_detach(tid[connections]);
        connections++;
    } 

    close(socket_fd);
    printf("Closing down!!\n");
    exit(0);
} 


void disconnectPeerFromHub(char *peer_ip){
	/*YET TO WRITE*/
    remove_entry(peer_ip ,hashtable);

}

char * searchHub(char *filename){
	 
	//char *seeder_ip = "127.0.0.1";
    pthread_mutex_lock(&mtx);
    char *seeder_ip = ht_get(hashtable,filename);
    pthread_mutex_unlock(&mtx);
    printf("\nAfter searching hub %s\n",seeder_ip);
	return seeder_ip;
}

void * doStuffWithClient(void *clientData)
{       
        
        client_info* info = ((client_info *)(clientData));
        int n;

        printf("Running thread no %d",info->thread_no);

        char buff[30]; 
        strcpy(buff, "Welcome ");
        strcat(buff, info->peer_ip);

        if ( (n =(send(info->client_fd,buff,strlen(buff),0))) == -1) 
        {
            fprintf(stderr, "Failure Sending Message\n");
            close(info->client_fd);
            return;
        }

        printf("\nHUB:Number of bytes sent: %d to %s\n",n,info->peer_ip);

        /*Recieve file list from peer and store it in data structure alongwith IP*/
        recieveFileList(info->client_fd,info->peer_ip);
        
        printf("\nState of hashtable after recieving from %s",info->peer_ip);
        pthread_mutex_lock(&mtx);
        print_hashtable(hashtable);
        pthread_mutex_unlock(&mtx);

        while(1){

        	int choice;
        	read(info->client_fd,&choice,sizeof(choice));

        	if(choice == 1){
        		char filename[24];

        		/*recieve the name of file client wants */
        		read(info->client_fd,filename,sizeof(filename));
        		char *seeder_ip;
        		seeder_ip = searchHub(filename);

        		/* Send the ip of seeder of that file*/
        		write(info->client_fd,seeder_ip,strlen(seeder_ip)+1);

        	}else if(choice == 2){
        		disconnectPeerFromHub(info->peer_ip);
        		pthread_exit(NULL);

        	}else{
        			//do nothing in case of invalid choice
        	}
        	
        }

}

int recieveFileList(int client_fd ,char * peer_ip)
{
    char fileList[100];
    int n = read(client_fd,fileList,sizeof(fileList));
    fileList[n] = '\0';
    //printf("\nRecieved fileList $%s$\n",fileList);
    char *line;
    int i,prev;
    prev = 0;

    printf("\nStoring files from %s:",peer_ip);
    for(i = 0; i < n; i++)
    {

        if(fileList[i] == '\n'){
            fileList[i] = '\0';
            line = &fileList[prev];
            
            pthread_mutex_lock(&mtx);
            ht_set( hashtable, line , peer_ip);
            pthread_mutex_unlock(&mtx);

            printf("\nStoring %s",line);
            prev = i+1;
        }
    }
    printf("\nSharing Successfull\n");
    
    //print_hashtable(hashtable);

    return 0;
}

void quit()
{  
    printf("\nctrl-c caught:\nTRY NOT Shutting down\n");
    //close(socket_fd);
    //exit(0);
}
