// ================= subscriber.c =================
#include "helpers.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "helpers.h"

#define MAX_TOPIC_LEN 50
#define MAX_ID_LEN 10


typedef struct {
    uint8_t type;
    char topic[MAX_TOPIC_LEN + 1];
    char client_id[MAX_ID_LEN + 1];
} tcp_msg;

void put_initial_id(int sockfd, const char *id) {
    tcp_msg m = {0xFF, "", ""};
    strncpy(m.client_id, id, MAX_ID_LEN);
    send(sockfd, &m, sizeof(m), 0);
}

void client(int sockfd, const char *client_id) {
    int epfd = epoll_create1(0);
    DIE(epfd == -1, "epoll_create");

    struct epoll_event ev_stdin = {.events = EPOLLIN, .data.fd = STDIN_FILENO};
    struct epoll_event ev_sock = {.events = EPOLLIN, .data.fd = sockfd};
    struct epoll_event events[2];

    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev_stdin);
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &ev_sock);

    char input_buf[BUFLEN];
    tcp_msg msg;

    while (1) {
        int n = epoll_wait(epfd, events, 2, -1);
        DIE(n < 0, "epoll_wait");

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == STDIN_FILENO) {
                memset(&msg, 0, sizeof(msg));
                fgets(input_buf, BUFLEN - 1, stdin);

                if (strncmp(input_buf, "exit", 4) == 0) {
                    msg.type = 2;
                    strncpy(msg.client_id, client_id, MAX_ID_LEN);
                    send(sockfd, &msg, sizeof(msg), 0);
                    close(epfd);
                    return;
                } else if (strncmp(input_buf, "subscribe ", 10) == 0) {
                    msg.type = 0;
                    strncpy(msg.client_id, client_id, MAX_ID_LEN);
                    strncpy(msg.topic, input_buf + 10, MAX_TOPIC_LEN);
                    msg.topic[strcspn(msg.topic, "\n")] = '\0';
                    send(sockfd, &msg, sizeof(msg), 0);
                    printf("Subscribed to topic %s\n", msg.topic);
                } else if (strncmp(input_buf, "unsubscribe ", 12) == 0) {
                    msg.type = 1;
                    strncpy(msg.client_id, client_id, MAX_ID_LEN);
                    strncpy(msg.topic, input_buf + 12, MAX_TOPIC_LEN);
                    msg.topic[strcspn(msg.topic, "\n")] = '\0';
                    send(sockfd, &msg, sizeof(msg), 0);
                    printf("Unsubscribed from topic %s\n", msg.topic);
                }

            } else if (events[i].data.fd == sockfd) {
                char recv_buf[BUFLEN];
                int r = recv(sockfd, recv_buf, sizeof(recv_buf) - 1, 0);
                if (r <= 0) {
                    close(epfd);
                    return;
                }
                recv_buf[r] = '\0';
                printf("%s", recv_buf);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    DIE(argc != 4, "Usage: ./subscriber <ID_CLIENT> <IP_SERVER> <PORT>");

    const char *client_id = argv[1];
    const char *server_ip = argv[2];
    int port = atoi(argv[3]);

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(sockfd < 0, "socket");

    int flag = 1;
    setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(port);
    inet_aton(server_ip, &serv.sin_addr);

    int rc = connect(sockfd, (struct sockaddr *)&serv, sizeof(serv));
    DIE(rc < 0, "connect");

    put_initial_id(sockfd, client_id);
    client(sockfd, client_id);

    close(sockfd);
    return 0;
}
