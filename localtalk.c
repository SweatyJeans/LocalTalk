#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <net/if.h>

#define BUFFER_SIZE 1024
#define PORT "8080"

int main(int argc, char** argv) {
    fputc('\n', stdout);
    if (argc < 2) {
        fprintf(stderr, "error: needs -s (send) or -l (listen) as arguments.\n");
        return 1;
    }
    char buffer[BUFFER_SIZE];
    int socketListen = -1, socketClient = -1, socketRemote = -1;
    if (strncmp(argv[1], "-s", 2) == 0) {
        //send
        char ipBuffer[INET6_ADDRSTRLEN];
        printf("\nEnter the IP you want to connect to (IPv4 and IPv6) > ");
        fgets(ipBuffer, sizeof(ipBuffer), stdin);
        ipBuffer[strcspn(ipBuffer, "\n")] = '\0';
        fputc('\n', stdout);

        struct addrinfo hints, *remoteAddress;

        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(ipBuffer, PORT, &hints, &remoteAddress)) {
            fprintf(stderr, "error > Address (%s) cannot be resolved (Code: %d)\n", ipBuffer, errno);
            goto forceEnd;
        }

        socketRemote = socket(remoteAddress->ai_family, remoteAddress->ai_socktype, remoteAddress->ai_protocol);
        if (socketRemote < 0) {
            fprintf(stderr, "error > Creating socket failed (Code: %d)\n", errno);
            freeaddrinfo(remoteAddress);
            goto forceEnd;
        }

        char remoteAddrBuffer[100];
        char remoteServiceBuffer[100];
        getnameinfo(remoteAddress->ai_addr, remoteAddress->ai_addrlen, remoteAddrBuffer, sizeof(remoteAddrBuffer), remoteServiceBuffer, sizeof(remoteServiceBuffer), NI_NUMERICHOST);
        printf("STATUS:\nConnecting to %s on port %s\n", remoteAddrBuffer, remoteServiceBuffer);

        if (connect(socketRemote, remoteAddress->ai_addr, remoteAddress->ai_addrlen)) {
            fprintf(stderr, "error > Connection attempt to %s failed (Code: %d)\n", remoteAddrBuffer, errno);
            freeaddrinfo(remoteAddress);
            goto forceEnd;
        }
        freeaddrinfo(remoteAddress);

        memset(buffer, 0, BUFFER_SIZE);
        int tmp;
        printf("STATUS > Waiting for connection authorization...\n");
        if ((tmp = recv(socketRemote, buffer, BUFFER_SIZE, 0)) == -1) {
            close(socketRemote);
            goto forceEnd;
        }

        buffer[tmp] = '\0';
        printf("\n%s\n", buffer);
        

        while (1) {
            memset(buffer, 0, BUFFER_SIZE);
            printf("STATUS > to TERMINATE the connection enter \"$end\"\nData to send > ");
            fflush(stdout);
            fgets(buffer, BUFFER_SIZE, stdin);
            fputc('\n', stdout);
            
            if (strncmp(buffer, "$end", 4) == 0) {
                close(socketRemote);
                break;
            }
            int bytesSent = send(socketRemote, buffer, strlen(buffer), 0);
            if (bytesSent <= 0) {
                fprintf(stderr, "error > sending data failed (Code: %d)\n", errno);
                continue;
            }
        }
    }
    else if (strncmp(argv[1], "-l", 2) == 0) {
        //listen
        struct addrinfo hints, *bindAddress;
        memset(&hints, 0, sizeof(hints));

        hints.ai_socktype = SOCK_STREAM;
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_PASSIVE;

        if (getaddrinfo(0, PORT, &hints, &bindAddress) != 0) {
            fprintf(stderr, "error: Can't switch to listening mode (Code: %d)\n", errno);
            freeaddrinfo(bindAddress);
            return 1;
        }

        socketListen = socket(bindAddress->ai_family, bindAddress->ai_socktype, bindAddress->ai_protocol);
        if (socketListen <= 0) {
            fprintf(stderr, "error: Can't create socket (Code: %d)\n", errno);
            freeaddrinfo(bindAddress);
            goto forceEnd;
        }

        if (bind(socketListen, bindAddress->ai_addr, bindAddress->ai_addrlen)) {
            fprintf(stderr, "error: Can't bind address (Code: %d)\n", errno);
            freeaddrinfo(bindAddress);
            goto forceEnd;
        }

        if (listen(socketListen, 3) < 0) {
            fprintf(stderr, "error: Can't activate listening mode (Code: %d)\n", errno);
            goto forceEnd;
        }

        freeaddrinfo(bindAddress);

        char ipstr[INET6_ADDRSTRLEN];
        struct ifaddrs* ifaddr, *ifa;

        if (getifaddrs(&ifaddr) == -1) {
            fprintf(stderr, "error: Can't get local IP addresses (Code: %d)\n", errno);
            freeifaddrs(ifaddr);
            goto forceEnd;
        }
        
        //Code for finding first usable IP address to connect to from another device
        int found = 0;
        for (ifa=ifaddr; ifa!=NULL; ifa=ifa->ifa_next) {
            if (!ifa->ifa_addr) continue;

            if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_RUNNING)) continue;
            if (ifa->ifa_flags & IFF_LOOPBACK) continue;
            if (strncmp(ifa->ifa_name, "docker", 6) == 0 || strncmp(ifa->ifa_name, "vbox", 4) == 0) continue;

            int family = ifa->ifa_addr->sa_family;
            void* addrPtr = NULL;

            if (family == AF_INET) {
                struct sockaddr_in* sa = (struct sockaddr_in*)ifa->ifa_addr;
                addrPtr = &(sa->sin_addr);

                // Link-local IPv4 vermeiden (169.254.x.x)
                unsigned char* bytes = (unsigned char*)&(sa->sin_addr);
                if (bytes[0] == 169 || bytes[1] == 254) continue;
            }
            else if (family == AF_INET6) {
                struct sockaddr_in6* sa6 = (struct sockaddr_in6*)ifa->ifa_addr;
                addrPtr = &(sa6->sin6_addr);

                // optional: Link-local IPv6 vermeiden (fe80::/10)
                if (IN6_IS_ADDR_LINKLOCAL(&sa6->sin6_addr)) continue;
            }
            else continue;

            if (getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6), ipstr, sizeof(ipstr), NULL, 0, NI_NUMERICHOST)) continue;

            if (strcmp(ipstr, "127.0.0.1") != 0 && strcmp(ipstr, "::1") != 0) {
                printf("\nSTATUS:\n\nFirst usable IP to connect to > %s\n", ipstr);
                found = 1;
                break;
            }
        }

        if (!found) {
            fprintf(stderr, "error: no usable IP address found.\n");
            freeifaddrs(ifaddr);
            goto forceEnd;
        }

        freeifaddrs(ifaddr);

        retryAccept:
        printf("\nWaiting for incoming connections...\n");
        struct sockaddr_storage clientAddress;
        socklen_t clientAddressLen = sizeof(clientAddress);

        socketClient = accept(socketListen, (struct sockaddr*)&clientAddress, &clientAddressLen);
        if (socketClient < 0) {
            fprintf(stderr, "Can't accept incoming connection (Code: %d)\n", errno);
            goto retryAccept;
        }

        void* addrPtr;
        if (clientAddress.ss_family == AF_INET) {
            addrPtr = &((struct sockaddr_in*)&clientAddress)->sin_addr;
        }
        else if (clientAddress.ss_family == AF_INET6) {
            addrPtr = &((struct sockaddr_in6*)&clientAddress)->sin6_addr;
        }
        else {
            fprintf(stderr, "error: Incoming connection is not IPv4 or IPv6.\n");
            goto retryAccept;
        }

        memset(ipstr, 0, sizeof(ipstr));
        inet_ntop(clientAddress.ss_family, addrPtr, ipstr, sizeof(ipstr));

        printf("Connection attempt from: %s\nAccept connection?(y/n) > ", ipstr);
        char answer = fgetc(stdin);
        
        if (answer != 'y' && answer != 'Y') {
            printf("STATUS > Connection was rejected.\n");
            close(socketClient);
            goto forceEnd;
        }

        printf("STATUS > Connection from %s accepted.\n", ipstr);
        memset(buffer, 0, BUFFER_SIZE);
        strcpy(buffer, "Connection attempt was accepted");
        send(socketClient, buffer, strlen(buffer), 0);

        while (1) {
            ssize_t bytesReceived = recv(socketClient, buffer, BUFFER_SIZE-1, 0);
            if (bytesReceived <= 0) {
                printf("STATUS > Connection from %s was terminated\n", ipstr);
                goto forceEnd;
            }

            buffer[bytesReceived] = '\0';

            printf("\nReceived data > %s", buffer);
        }
    }
    else {
        fprintf(stderr, "error: undefined parameter <%s>\n", argv[1]);
        return 1;
    }

    //end label
    forceEnd:
    if (socketListen != -1) close(socketListen);
    if (socketClient != -1) close(socketClient);
    if (socketRemote != -1) close(socketRemote);
    printf("STATUS > Program execution successfully completed\n");
    return 0;
}