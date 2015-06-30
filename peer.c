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
#include <fcntl.h>
#include <signal.h>

#define PORT 3490
#define HUB_IP "127.0.0.1"//"10.20.17.207"//"10.20.10.201"
#define MAXSIZE 1024

int sendFileList(char *path,int socket_fd); //send the peer's files to hub
void quit();

int main(int argc, char *argv[])
{

	signal(SIGINT,quit); /* trap ctrl-c call quit fn */

    /*Connect to HUB*/
    struct sockaddr_in hub_info;
    struct hostent *he;
    int socket_fd;
    char buff[50];

    if ((he = gethostbyname(HUB_IP))==NULL) {
        fprintf(stderr, "Cannot get host name\n");
        exit(1);
    }

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1) {
        fprintf(stderr, "Socket Failure!!\n");
        exit(1);
    }

    memset(&hub_info, 0, sizeof(hub_info));
    hub_info.sin_family = AF_INET;
    hub_info.sin_port = htons(PORT);
    hub_info.sin_addr = *((struct in_addr *)he->h_addr);

    if (connect(socket_fd, (struct sockaddr *)&hub_info, sizeof(struct sockaddr))<0) {   
        perror("connect");
        exit(1);
    }else{
        printf("\nConnection to hub successful\n");
    }


    int num = recv(socket_fd, buff, sizeof(buff),0);
    if ( num <= 0 )
    {
       printf("Either Connection Closed or Error\n");
    }

    buff[num] = '\0';
    printf("\nClient:Message Received From HUB - %s\n",buff);

    /*Storing the peer's share in file_list.txt*/
    char path[50]; //path of the folder which peer intends to share
    char download_path[50];
    int share_success = 1;
    while(share_success != 0)
    {
    	printf("\nEnter path of folder to be shared>");
    	scanf("%s",path);
    	printf("\nEnter path of folder to download files>");
    	scanf("%s",download_path);
    	printf("\nSharing files...\n");

    	/*send file list*/
    	share_success = sendFileList(path,socket_fd);
    }
    	
    /*Now that share is successful peer is ready to seed*/
    pid_t pid_seeder;
    
    if((pid_seeder = fork()) == -1) {       // test fork and assign child pid
            perror("fork failed\n");
    }

    if(pid_seeder == 0){

        /*Child process will seed*/
        printf("Seeders path %s",path);
        if(execl("./seed","seed",path,(char *)0) == -1){
            perror("execl of send failed");
        }

    }

    /*Parent will either leech or do nothing*/
    while(1)
    {
        int choice;
        printf("\nEnter 1 to download or 2 to exit>");
        scanf("%d",&choice);

        /*Send hub the clients choice*/
        if(num = write(socket_fd, &choice, sizeof(choice)) <= 0){
            perror("\nError in sending choice");
        }

        if(choice == 1){

	       	int status = 1;
            while(status != 0)
            {
               	if(status == 256){
              		printf("\nRetrying to connect\n");
                }

                printf("\nEnter name of file to download>");
                char filename[24];
                scanf("%s",filename);
                if(write(socket_fd, filename, sizeof(filename)) < 0){
                	perror("\nError in sending filename to hub");
                }

                char seeder_ip[20];
                if(read(socket_fd, seeder_ip, sizeof(seeder_ip)) < 0){
                	perror("\nError in recieving seeder ip from hub");
                }
                printf("\nRecieving from %s\n",seeder_ip );
                
                printf("\nSave File as>");
                char file_save_as_name[24];
                scanf("%s",file_save_as_name);

                //signal(SIGCHLD, SIG_IGN); /* Silently (and portably) reap children. */
                pid_t pid_leecher;
                if((pid_leecher = fork()) == -1)       // test fork and assign child pid
                   	perror("fork failed 2\n");
                

                if(pid_leecher == 0){
                   	/*This child will leech*/
                   	if(execl("./rec","rec",filename,download_path,file_save_as_name,seeder_ip,(char *)0) == -1)
                   		perror("\nRec execl failed!");

                }else{
                    
                    waitpid(pid_leecher,&status,0);
                    printf("\nstatus of leecher is :%d\n",status);
                }
            }

        }else if(choice == 2){
                
                /*int status;
                kill(pid_seeder, SIGTERM); //this gives the child an opportunity to terminate gracefully
                sleep(10);
                waitpid(pid_seeder, &status, WNOHANG);
                kill(pid_seeder, SIGKILL);
                
                waitpid(pid_seeder, &status, 0);
                printf("\nstatus of seeder is :%d\n",status);*/
                close(socket_fd);
                break;

        }else{
                printf("Invalid input");
        }
    }
    //close(socket_fd);
    return 0;

}


int sendFileList(char *path,int socket_fd){
    
    /* Save current stdout and stderr for use later */
    int saved_stdout = dup(STDOUT_FILENO);
    //printf("%d %d",saved_stdout,STDOUT_FILENO);
    //int saved_stderr = dup(STDERR_FILENO);
    
    dup2(socket_fd,STDOUT_FILENO); /*copy the file descriptor fd into standard output*/
    //dup2(fd,STDERR_FILENO); /* same, for the standard error */
    //close(fd); /* close the file descriptor as we don't need it more  */

    /*execl ls */
    pid_t pid_ls;
    if((pid_ls = fork()) == 0){
        execl( "/bin/ls" , "ls" , path , (char *) 0 );
    }

    
    
    /* Restore stdout and stderr */
    dup2(saved_stdout,STDOUT_FILENO);
    close(saved_stdout);
    /*dup2(saved_stderr, STDERR_FILENO);
    close(saved_stderr);*/

    //close(fd);

    int status;
    waitpid(pid_ls,&status,0);
    if(status != 0){
    	printf("\nDirectory doesn't exist");
    	return 1;
    }
    printf("\nstatus of ls :%d\n",status);
    printf("Share successful\n");
    return 0;
}


void quit()
{  
    printf("\nctrl-c caught:\nShutting down\n");
    //close(socket_fd);
    exit(0);
}
