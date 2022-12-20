#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/tcp.h>
#include <time.h>

#define SERVER_PORT 5064
#define SERVER_IP_ADDRESS "127.0.0.1"
#define FILE_SIZE 1078441
#define BUFFER_SIZE 4096

int main()
{
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 1
    if (sock == -1)
    {
        printf("Could not create socket : %d", errno);
        return -1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    int k=0;

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);                                            
    int rval = inet_pton(AF_INET, (const char *)SERVER_IP_ADDRESS, &serverAddress.sin_addr); 
    
    if (rval <= 0)
    {
        printf("inet_pton() failed");
        return -1;
    } 

    // Make a connection to the server with socket SendingSocket.
    int connectResult = connect(sock, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (connectResult == -1)
    {
        printf("connect() failed with error code : %d", errno);
        // close the socket;
        close(sock);
        return -1;
    } 
loop:

    printf("connected to server..\n");
    // change CC algorithm to cubic
    char buf[256];
    printf("Changed Congestion Control to Cubic\n");
    strcpy(buf, "cubic");
    socklen_t socklen = strlen(buf);
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, socklen) != 0)
    {
        perror("failed in setting of cubic");
        return -1;
    }
    socklen = sizeof(buf);
    if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &socklen) != 0)
    {
        perror("failed in getting the curr CC");
        return -1;
    }

    // define the file name and then open and reading the file
    FILE *file;

    file = fopen("check.txt", "r");
    int sum = 0, b;
    char data[BUFFER_SIZE] = {0};
    while (((b = fread(data, 1, sizeof data, file)) > 0) && (sum < (FILE_SIZE / 2)+1))
    {
        if (send(sock, data, sizeof(data), 0) == -1)
        {
            perror("failed in sending\n");
            exit(1);
        }
        sum += b;
        bzero(data, BUFFER_SIZE);
    }
    printf("finished sending first half amount sent is: %d \n", sum);
    char *message = "finish";
    int bytesSent = send(sock, message, strlen(message) + 1, 0);
    if (bytesSent == -1)
    {
        printf("send failed with error code : %d", errno);
        close(sock);
        return -1;
    }
    // waiting to authntication 
    printf("waiting for server to send authuntication...\n");
    char bufferReply[BUFFER_SIZE] = {'\0'};
    int bytesReceived = recv(sock, bufferReply, BUFFER_SIZE, 0);
    if (bytesReceived == -1)
    {
        printf("recv() failed with error code.");
    }
    else if (bytesReceived == 0)
    {
        printf("peer has closed the TCP connection prior to recv().\n");
    }
    else
    {
        printf("received %d bytes from server. reply: %s\n", bytesReceived, bufferReply);
    }
    int flag = 1;
    char xor [] = "1101000001000010";
    for (int i = 0; i < strlen(xor); i++)
    {
        if (xor[i] != bufferReply[i])
            flag = 0;
    }
    if (flag == 0)
    {
        printf("authntication failed\n");
        close(sock);
        return 1;
    }
    
    // Changing CC algo to reno 
    printf("Changed Congestion Control to Reno\n");
    strcpy(buf, "reno");
    socklen = strlen(buf);
    if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, socklen) != 0)
    {
        perror("failed in setting the sock");
        return -1;
    }
    socklen = sizeof(buf);
    if (getsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, buf, &socklen) != 0)
    {
        perror("failed in setting the sock");
        return -1;
    }

    // reading the second half
    int prevsum = sum;
    int x;
    bzero(data, BUFFER_SIZE);
    while ((b = fread(data, 1, BUFFER_SIZE, file)) > 0)
    {
        if ((x = send(sock, data, b, 0)) == -1)
        {
            printf("sending failed\n");
            return -1;
        }
        sum += b;
        // bzero(data, BUFFER_SIZE);
    }
    fclose(file);
    
    char *message2 = "finish";
    bytesSent = send(sock, message2, strlen(message2) + 1, 0);
    if (bytesSent == -1)
    {
        printf("send() failed with error code : %d", errno);
        close(sock);
        return -1;
    }
    printf("amount sent in half 2 is %d. \n", (sum - prevsum));
    if (k<5)
    {
        k++; 
        printf("****************************************************\n");
        goto loop;
    }
    
    

    // USER DECISION
    char buffer[BUFFER_SIZE] = {0};
    printf("Enter \"exit\" to exit, enter \"n\" to restart the process: \n");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strlen(buffer) - 1] = '\0';
    bytesSent = send(sock, buffer, strlen(buffer) + 1, 0); // 4

    if (bytesSent == -1)
    {
        printf("send() failed with error code : %d", errno);
    }
    else if (bytesSent == 0)
    {
        printf("peer has closed the TCP connection prior to send().\n");
    }
    else if (bytesSent < strlen(buffer) + 1)
    {
        printf("sent only %d bytes from the required %d.\n", BUFFER_SIZE, bytesSent);
    }
    else
    {
        printf("sendding the message succesfuly: %s \n", buffer);
    }
    if (strncmp(buffer, "exit", 1) != 0)
    {
        bzero(buffer, BUFFER_SIZE);
        printf("****************************************************\n");
        goto loop;
    }
    close(sock);
    return 0;
}