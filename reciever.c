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
#include <sys/time.h>

#define SERVER_PORT 5064 // The port that the server listens
#define BUFFER_SIZE 4096
#define FILE_SIZE 1078441

int main()
{
    // Open a listening socket
    int listeningSocket = -1;
    listeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // 0 means default protocol for stream sockets (Equivalently, IPPROTO_TCP)
    if (listeningSocket == -1)
    {
        printf("Couldn't create listening socket : %d", errno);
        return 1;
    }

    // Reuse the address if the server socket on was closed
    // and remains for 45 seconds in TIME-WAIT state till the final removal.
    int enableReuse = 1;
    int ret = setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &enableReuse, sizeof(int));
    if (ret < 0)
    {
        printf("failed in setting the socket with error : %d", errno);
        return 1;
    }

    // "sockaddr_in" is the "derived" from sockaddr structure
    // used for IPv4 communication. For IPv6, use sockaddr_in6
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    //set the time
    struct timeval start, end;
    long tot = 0;
    int k=0;

    serverAddress.sin_addr.s_addr = INADDR_ANY;//will accept any IP in this port
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT); // network bytes order/Big Endian

    // Bind the socket 
    int bindResult = bind(listeningSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    if (bindResult == -1)
    {
        printf("Bind failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    printf("Bind() success\n");

    // Make the socket listening; actually mother of all client sockets.
    // 500 is a Maximum size of queue connection requests
    // number of concurrent connections
    int listenResult = listen(listeningSocket, 3);
    if (listenResult == -1)
    {
        printf("listen() failed with error code : %d", errno);
        // close the socket
        close(listeningSocket);
        return -1;
    }

    
    struct sockaddr_in clientAddress; //
    socklen_t clientAddressLen = sizeof(clientAddress);

    while (1)
    {
        printf("Waiting for incoming TCP-connections...\n");
        // Accept and incoming connection
        memset(&clientAddress, 0, sizeof(clientAddress));
        clientAddressLen = sizeof(clientAddress);
        int clientSocket = accept(listeningSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            printf("listen failed with error code : %d\n", errno);
            // close the sockets
            close(listeningSocket);
            return -1;
        }
        printf("A new client connection accepted\n");
        //variables for measure the time
        char box[100000] = "";
        long tothalf1 = 0;
        long tothalf2 = 0;
        int countotfile = 1;

    loop:
        printf("round number %d to send file: ", countotfile);

        // code got changing CC algorithm
        char buf[256];
        printf("Changed Congestion Control to cubic\n");
        strcpy(buf, "cubic");
        socklen_t socklen = strlen(buf);

        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buf, socklen) != 0)
        {
            perror("failed in setting CC to cubic");
            return -1;
        }
        if (getsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buf, &socklen) != 0)
        {
            perror("failed in getting CC to cubic");
            return -1;
        }

        // time capturing handling
        // Receive a message from client
        char buf1[BUFFER_SIZE];
        bzero(buf1, BUFFER_SIZE);
        int bytesReceived, sum = 0;
        gettimeofday(&start, NULL);
        while ((bytesReceived = recv(clientSocket, buf1, BUFFER_SIZE, 0)) > 0)
        {
            if (strstr(buf1, "inish") != NULL)
            {
                //printf("Received %d bytes from client: %s\n", bytesReceived, buf1);
                break;
            }
            else
            {
                //printf("recieve %d bytes from sender\n",bytesReceived);
                sum += bytesReceived;
                bzero(buf1, BUFFER_SIZE);
            }
        }
    
        gettimeofday(&end, NULL);
        printf("recieves in total for first half: %d bytes \n", sum);
        // time capture
        tot = ((end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
        tothalf1 += tot;
        char temp_str[50] = "time in micro seconds for the first half: ";

        char temp[20];
        sprintf(temp, "%ld", tot);
        strcat(temp_str, temp);
        strcat(temp_str, "\n");
        strcat(box, temp_str);

        // Reply to client
        printf("sending authuntication to client...\n");

        char *message = "1101000001000010"; //xor on the last 4 digits in each ID 
        int messageLen = strlen(message) + 1;

        int bytesSent = send(clientSocket, message, messageLen, 0);
        if (bytesSent == -1)
        {
            printf("failed to send with error code : %d", errno);
            close(listeningSocket);
            close(clientSocket);
            return -1;
        }
        else if (bytesSent == 0)
        {
            printf("peer has closed the TCP connection prior to send().\n");
        }
        else if (bytesSent < messageLen)
        {
            printf("sent only %d bytes from the required %d.\n", messageLen, bytesSent);
        }

        // Changing to reno algorithm
        printf("Changed CC algo to reno\n");
        strcpy(buf, "reno");
        socklen = strlen(buf);
        if (setsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buf, socklen) != 0)
        {
            perror("failed in setting CC algo to reno");
            return -1;
        }
        socklen = sizeof(buf);
        if (getsockopt(listeningSocket, IPPROTO_TCP, TCP_CONGESTION, buf, &socklen) != 0)
        {
            perror("failed in getting the CC algo");
            return -1;
        }
        

        // waiting to recieve second part.
        bzero(buf1, BUFFER_SIZE);
        gettimeofday(&start, NULL);
        while ((bytesReceived = recv(clientSocket, buf1, BUFFER_SIZE, 0)) > 0)
        {
            if (strstr(buf1, "inish") != NULL)
            {
                //printf("Received %d bytes from client: %s\n", bytesReceived, buf1);   
                break;
            }
            else
            {
                sum += bytesReceived;
                bzero(buf1, BUFFER_SIZE);
            }
        }
    
        // time capture
        gettimeofday(&end, NULL);
        tot = ((end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
        tothalf2 += tot;

        printf("in total for second half: recieved: %d bytes \n", sum);
        char temp_str1[50] = "time in micro seconds that taken to second half: ";
        char temp1[20];
        sprintf(temp1, "%ld", tot);
        strcat(temp_str1, temp1);
        strcat(temp_str1, "\n");
        strcat(box, temp_str1);

        if (k<5)
        {
            k++;
            countotfile++;
                printf("****************************************************\n");
                goto loop;
        }
        

        // USER DECISION
        printf("waiting for user decision...\n");
        memset(buf1, 0, BUFFER_SIZE);
        if ((bytesReceived = recv(clientSocket, buf1, BUFFER_SIZE, 0)) > 0)
        {
            printf("Received %d bytes from client. decision is %s\n", bytesReceived, buf1);

            // check if got the "exit" command from client, if yes, then exit and close the client socket
            if (strncmp(buf1, "exit", 1) == 0)
            {
                printf("Client has decided to end the session. Stats:\n");
                puts(box);
                printf("Total time for first half in file[%d] is: %ld for %d times for an avarage of %ld. \n", countotfile, tothalf1, countotfile, tothalf1 / countotfile);
                printf("Total time for second half in file[%d] is: %ld for %d times for an avarage of %ld. \n", countotfile, tothalf2, countotfile, tothalf2 / countotfile);
                // close(clientSocket);
            }
            else
            {
                countotfile++;
                printf("****************************************************\n");
                goto loop;
            }
        }
    }
    close(listeningSocket);
    return 0;
}