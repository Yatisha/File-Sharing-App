#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define PORT 55550

void doStuff(int connfd,char *path);

void error(const char *msg)
{
    //perror(msg);
    printf("%s : %s\n",msg,strerror(errno));
    exit(1);
}

int main(int argc, char *argv[])
{
    if(argc != 2){
        printf("\nNot enough arguments: specify path of shared folder\n");
    }

    char path[50];
    strcpy(path,argv[1]);
    printf("\nSeeder beginning %s\n",path );
    int listenfd = 0;
    int connfd = 0;
    struct sockaddr_in serv_addr;
    char sendBuff[1025];
    int numrv;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    printf("\nSocket created to share \n");

    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(sendBuff, '0', sizeof(sendBuff));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    bind(listenfd, (struct sockaddr*)&serv_addr,sizeof(serv_addr));

    if(listen(listenfd, 10) == -1)
    {
        error("\nFailed to listen\n");
    }else{
        printf("\nListening to share\n");
    }

    
    while(1)
    {
    	size = sizeof(struct sockaddr_in);
    	struct sockaddr client;
        connfd = accept(listenfd, (struct sockaddr *)&client, &size));
        if(connfd == -1){
            error("\nError in accept\n");
        }else{
            printf("\nSeeder:Successfully connected to peer\n");
        }

        //signal(SIGCHLD, SIG_IGN); /* Silently (and portably) reap children. */
        int pid = fork();
        if (pid < 0)
            error("\nERROR on fork");

        if (pid == 0)
        {
            close(listenfd);
            doStuff(connfd,path);
            exit(0);

        }else{
            close(connfd);
        }
    }


    return 0;
}

void doStuff(int connfd,char * path){

        /* read the name of the file that peer wants*/
        char filename[24];
        int n = read(connfd,filename, 24);
        if(n <= 0){
            error("\nerror in reading name of file");
        }
        filename[n] = '\0';
        
        printf("\nfilename: %s",filename );
        printf("\npath:%s",path);
        char complete_path[50];
        strcpy(complete_path,"./");
        strcat(complete_path,path);
        strcat(complete_path,"/");
        strcat(complete_path,filename);        

        /* Open the file that we wish to transfer */
        
        printf("\nSeeder:Sending file %s\n",complete_path);
        
        FILE *fp = fopen(complete_path,"rb");
        
        if(fp==NULL)
        {
            error("\nFile open error");   
        }else{
            printf("\nSeeder:File ready to be sent\n");
        }   
        
        /* read the size of the portion of file that peer already has */
        int sz;
        if(read(connfd,&sz,sizeof(sz)) < 0){
            error("\nSeeder:Error in reading file size");
        }
        
        
        /* Seek the file pointer to the chunk peer wants*/
        fseek(fp,sz,SEEK_SET);
        printf("\nSeek success\n");
        /* Read data from file and send it */
        while(1)
        {
            /* First read file in chunks of 256 bytes */
            unsigned char buff[256]={0};
            int nread = fread(buff,1,256,fp);
            printf("\nBytes read %d \n", nread);        

            /* If read was success, send data. */
            if(nread > 0)
            {
                printf("\nSending \n");
                write(connfd, buff, nread);
            }

            /*
             * There is something tricky going on with read .. 
             * Either there was error, or we reached end of file.
             */
            if (nread < 256)
            {
                if (feof(fp))
                    printf("\nEnd of file\n");
                if (ferror(fp))
                    printf("\nError reading\n");
                break;
            }

            
        }
        close(connfd); //close connection with leecher
        return;
}
