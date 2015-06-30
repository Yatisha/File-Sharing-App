#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 55550

void error(const char *msg)
{
    //perror(msg);
    printf("%s : %s\n",msg,strerror(errno));
    exit(1);
}

int main(int argc , char* argv[])
{
    if(argc != 5){
        printf("usage> ./rec file_name_on_seeder_side path_of_folder_to_store_file file_name_on_leecher_side seeder_ip");
        exit(0);
    }
    
    char *seeder_ip = argv[4];
    printf("Seeder ip is %s",seeder_ip);
    /*Start recieving from seeder on getting its ip*/
    int sockfd = 0;
    int bytesReceived = 0;
    char recvBuff[256];
    
    memset(recvBuff, '0', sizeof(recvBuff));
    struct sockaddr_in serv_addr;

    /* Create a socket first */
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))< 0)
    {
        error("\nError : Could not create socket \n");
    }else{
        printf("\nSocket created successfully to recieve from %s\n",seeder_ip );
    }

    /* Initialize sockaddr_in data structure */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT); // port
    serv_addr.sin_addr.s_addr = inet_addr(seeder_ip);
    //inet_pton(AF_INET,"10.20.10.105",&serv_addr.sin_addr);
    
    /* Attempt a connection */
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
        error("\nError: Connect Failed \n");
    }else{
        printf("\nConnection to seeder:%s successful\n",seeder_ip);
    }

    
    /* Send peer the name of file it wants */
    if(write(sockfd, argv[1], strlen(argv[1])) <= 0){
        error("Error in sending filename to seeder");
    }
    
    
    /* Create or open file where data will be stored */
    FILE *fp;
    char complete_path[50];
    strcpy(complete_path,"./");
    strcat(complete_path,argv[2]);
    strcat(complete_path,"/");
    strcat(complete_path,argv[3]);
    
    printf("\nLeecher:Saving file to %s\n",complete_path);

    fp = fopen(complete_path, "ab"); 
    if(NULL == fp)
    {
        printf("Error opening file");
        
        return 1;
    }
    printf("\ntarget file opened");
    /* Send peer the size of downloaded portion of desired file */
    fseek(fp, 0L, SEEK_END);    
    int sz = ftell(fp);
    fseek(fp, 0L, SEEK_SET); //seek back to the beginning of file
    if(write(sockfd, &sz, sizeof(sz)) < 0){
        error("\nError in sending file size");
    }
    printf("\nDone sending size of file");
    
    
    /* Receive data in chunks of 256 bytes */
    while((bytesReceived = read(sockfd, recvBuff, 256)) > 0)
    {
        printf("\nBytes received %d",bytesReceived);    
        // recvBuff[n] = 0;
        fwrite(recvBuff,1,bytesReceived,fp);
        // printf("%s \n", recvBuff);
    }

    if(bytesReceived < 0)
    {
        error("Error reading from seeder");
        exit(1);
    }

    fclose(fp); //close the file
    close(sockfd); //close connection with the seeder
    exit(0);
}
