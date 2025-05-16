#include "helpers.h"
#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define NR_MAX_FD 1024
#define NR_MAXIM_USER 100
#define NR_MAX_SUB 100

typedef struct user_data {
    char identifier[11];
    int socket_fd;
    int connected;
    char *subscribed_topics[NR_MAX_SUB];
    int subscription_count;
} inf_user;

inf_user lista_useri[NR_MAXIM_USER];

typedef struct udp_data {
    char subject[50];
    uint8_t content_type;
    char content[1500];
} udp_packet;

typedef struct tcp_data {
    uint8_t message_type;
    char subject[51];
    char identifier[11];
} tcp_packet;

int find_user(const char *id_ptr, int fd) {
    if (id_ptr != NULL) {
        for (int i = 0; i < NR_MAXIM_USER; ++i) {
            if (strcmp(lista_useri[i].identifier, id_ptr) == 0) {
                return i;
            }
        }
    } else if (fd != -1) {
        for (int i = 0; i < NR_MAXIM_USER; ++i) {
            if (lista_useri[i].connected && lista_useri[i].socket_fd == fd) {
                return i;
            }
        }
    }
    return -1;
}

int manage_user(int file_descriptor, const char *user_id) {
    int existing_user_index = find_user(user_id, -1);
    if (existing_user_index != -1) {
        if (lista_useri[existing_user_index].connected) {
            return -1;
        }
        lista_useri[existing_user_index].socket_fd = file_descriptor;
        lista_useri[existing_user_index].connected = 1;
        return 0;
    }

    for (int i = 0; i < NR_MAXIM_USER; ++i) {
        if (!lista_useri[i].connected && strlen(lista_useri[i].identifier) == 0) {
            lista_useri[i].connected = 1;
            lista_useri[i].socket_fd = file_descriptor;
            strncpy(lista_useri[i].identifier, user_id, 10);
            lista_useri[i].subscription_count = 0;
            return 0;
        }
    }
    return -1;
}

void treat_subs(int user_index, uint8_t message_type, const char *topic_str) {
    if (message_type == 0) {
        int is_subscribed = 0;
        for (int i = 0; i < lista_useri[user_index].subscription_count; ++i) {
            if (strcmp(lista_useri[user_index].subscribed_topics[i], topic_str) == 0) {
                is_subscribed = 1;
                break;
            }
        }
        if (!is_subscribed) {
            lista_useri[user_index].subscribed_topics[lista_useri[user_index].subscription_count++] = strdup(topic_str);
        }
    } else if (message_type == 1) {
        for (int i = 0; i < lista_useri[user_index].subscription_count; ++i) {
            if (strcmp(lista_useri[user_index].subscribed_topics[i], topic_str) == 0) {
                free(lista_useri[user_index].subscribed_topics[i]);
                lista_useri[user_index].subscribed_topics[i] = lista_useri[user_index].subscribed_topics[--lista_useri[user_index].subscription_count];
                return;
            }
        }
    }
}

double custom_power(int base, int exponent) {
    double result = 1.0;
    for (int i = 0; i < abs(exponent); ++i) {
        result *= base;
    }
    if (exponent >= 0) {
    return result;
} else {
    return 1.0 / result;
}

}

void format_udp_payload(char *output_buffer, struct sockaddr_in *client_address, udp_packet *udp_payload) {
    char type_description[20], formatted_value[1501] = {0};

    if (udp_payload->content_type == 0) {
        strcpy(type_description, "INT");
        int numeric_val = ntohl(*(int *)(udp_payload->content + 1));
        if (udp_payload->content[0]) numeric_val = -numeric_val;
        sprintf(formatted_value, "%d", numeric_val);
    } else if (udp_payload->content_type == 1) {
        strcpy(type_description, "SHORT_REAL");
        uint16_t raw_val = ntohs(*(uint16_t *)(udp_payload->content));
        sprintf(formatted_value, "%d.%02d", raw_val / 100, raw_val % 100);
    } else if (udp_payload->content_type == 2) {
        strcpy(type_description, "FLOAT");
        uint8_t is_negative = udp_payload->content[0];
        uint32_t mantissa_val = ntohl(*(uint32_t *)(udp_payload->content + 1));
        uint8_t exponent_val = udp_payload->content[5];
        double float_val = mantissa_val / custom_power(10, exponent_val);
        if (is_negative) float_val = -float_val;
        snprintf(formatted_value, sizeof(formatted_value), "%.*f", exponent_val, float_val);
    } else if (udp_payload->content_type == 3) {
        strcpy(type_description, "STRING");
        snprintf(formatted_value, sizeof(formatted_value), "%s", udp_payload->content);
    } else {
        strcpy(type_description, "UNKNOWN");
        strcpy(formatted_value, "-");
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address->sin_addr, ip_str, INET_ADDRSTRLEN);
    int port_num = ntohs(client_address->sin_port);

    snprintf(output_buffer, BUFLEN, "%s:%d - %s - %s - %s", ip_str, port_num, udp_payload->subject, type_description, formatted_value);
}

int manage_wildcars(const char *pattern_str, const char *topic_str) {
    char pat_copy[BUFLEN], topic_copy[BUFLEN];
    strncpy(pat_copy, pattern_str, BUFLEN - 1);
    strncpy(topic_copy, topic_str, BUFLEN - 1);
    pat_copy[BUFLEN - 1] = '\0';
    topic_copy[BUFLEN - 1] = '\0';

    char *pat_part[50], *topic_parts[50];
    int pat_part_cnt = 0, topic_part_count = 0;

    for (char *token = strtok(pat_copy, "/"); token; token = strtok(NULL, "/")) {
        pat_part[pat_part_cnt++] = token;
        if (pat_part_cnt >= 50) break; 
    }

    for (char *token = strtok(topic_copy, "/"); token; token = strtok(NULL, "/")) {
        topic_parts[topic_part_count++] = token;
        if (topic_part_count >= 50) break; 
    }

    // fac match
    int p_idx = 0, t_idx = 0;
    while (p_idx < pat_part_cnt && t_idx < topic_part_count) {
        if (strcmp(pat_part[p_idx], "*") == 0) {
            if (p_idx == pat_part_cnt - 1) {
                return 1; 
            }
            for (int k = t_idx; k < topic_part_count; k++) {
                char *next_pattern = pattern_str + (pat_part[p_idx + 1] - pat_copy);
                char *next_topic = topic_str + (topic_parts[k] - topic_copy);
                if (manage_wildcars(next_pattern, next_topic)) {
                    return 1;
                }
            }
            return 0;
        }
        else if (strcmp(pat_part[p_idx], "+") == 0) {
            p_idx++;
            t_idx++;
        }
        else if (strcmp(pat_part[p_idx], topic_parts[t_idx]) == 0) {
            p_idx++;
            t_idx++;
        }
        else {
            return 0;
        }
    }

    return (p_idx == pat_part_cnt && t_idx == topic_part_count) ||
           (p_idx == pat_part_cnt - 1 && strcmp(pat_part[p_idx], "*") == 0);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
    int tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    int enable = 1;
    setsockopt(tcp_socket_fd, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(int));

    int udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(tcp_socket_fd < 0, "tcp socket creation failed");
    DIE(udp_socket_fd < 0, "udp socket creation failed");

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(atoi(argv[1]));

    DIE(bind(tcp_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0, "tcp bind failed");
    DIE(bind(udp_socket_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0, "udp bind failed");
    DIE(listen(tcp_socket_fd, NR_MAXIM_USER) < 0, "listen failed");

    struct pollfd captured_fds[NR_MAX_FD];
    int num_fds = 0;

    captured_fds[num_fds++] = (struct pollfd){STDIN_FILENO, POLLIN, 0};
    captured_fds[num_fds++] = (struct pollfd){tcp_socket_fd, POLLIN, 0};
    captured_fds[num_fds++] = (struct pollfd){udp_socket_fd, POLLIN, 0};

    while (1) {
        int ready_count = poll(captured_fds, num_fds, -1);
        DIE(ready_count < 0, "poll error");

        for (int i = 0; i < num_fds; i++) {
            if (captured_fds[i].revents & POLLIN) {
                int fd = captured_fds[i].fd;
                if (fd == STDIN_FILENO) {
                    char buffer[10];
                    fgets(buffer, sizeof(buffer), stdin);
                    if (strncmp(buffer, "exit", 4) == 0) {
                        for (int u = 0; u < NR_MAXIM_USER; u++) {
                            if (lista_useri[u].connected) close(lista_useri[u].socket_fd);
                        }
                        close(tcp_socket_fd);
                        close(udp_socket_fd);
                        return 0;
                    }
                } else if (fd == tcp_socket_fd) {
                    struct sockaddr_in client_addr;
                    socklen_t addr_len = sizeof(client_addr);
                    int new_client_fd = accept(tcp_socket_fd, (struct sockaddr*)&client_addr, &addr_len);
                    DIE(new_client_fd < 0, "accept error");
                    setsockopt(new_client_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&enable, sizeof(int));
                    tcp_packet initial_packet;
                    recv(new_client_fd, &initial_packet, sizeof(initial_packet), 0);
                    if (manage_user(new_client_fd, initial_packet.identifier) < 0) {
                        printf("Client %s already connected.\n", initial_packet.identifier);
                        close(new_client_fd);
                    } else {
                        captured_fds[num_fds++] = (struct pollfd){new_client_fd, POLLIN, 0};
                        printf("New client %s connected from %s:%d.\n", initial_packet.identifier,
                               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    }
                } else if (fd == udp_socket_fd) {
                    udp_packet received_udp_packet;
                    struct sockaddr_in source_address;
                    socklen_t sock_len = sizeof(source_address);
                    recvfrom(udp_socket_fd, &received_udp_packet, sizeof(received_udp_packet), 0, (struct sockaddr*)&source_address, &sock_len);
                    char formatted_message[1600] = {0};
                    format_udp_payload(formatted_message, &source_address, &received_udp_packet);
                    size_t message_length = strlen(formatted_message);
                    if (message_length < sizeof(formatted_message) - 2) {
                        formatted_message[message_length] = '\n';
                        formatted_message[message_length + 1] = '\0';
                    }
                    for (int u = 0; u < NR_MAXIM_USER; u++) {
                        if (lista_useri[u].connected) {
                            int was_sent = 0;
                            for (int s = 0; s < lista_useri[u].subscription_count; s++) {
                                if (manage_wildcars(lista_useri[u].subscribed_topics[s], received_udp_packet.subject)) {
                                    if (!was_sent) {
                                        send(lista_useri[u].socket_fd, formatted_message, strlen(formatted_message), 0);
                                        was_sent = 1;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    tcp_packet received_tcp_packet;
                    int received_bytes = recv(fd, &received_tcp_packet, sizeof(received_tcp_packet), 0);
                    int user_index = find_user(NULL, fd);
                    if (received_bytes <= 0) {
                        if (user_index != -1) {
                            printf("Client %s disconnected.\n", lista_useri[user_index].identifier);
                            lista_useri[user_index].connected = 0;
                            close(fd);
                            for (int j = 0; j < num_fds; j++) {
                                if (captured_fds[j].fd == fd) {
                                    captured_fds[j] = captured_fds[num_fds - 1];
                                    num_fds--;
                                    break;
                                }
                            }
                        }
                    } else {
                        received_tcp_packet.subject[strcspn(received_tcp_packet.subject, "\n")] = '\0';
                        if (received_tcp_packet.message_type == 0 || received_tcp_packet.message_type == 1) {
                            treat_subs(user_index, received_tcp_packet.message_type, received_tcp_packet.subject);
                        } else if (received_tcp_packet.message_type == 2) {
                            if (user_index != -1) {
                                printf("Client %s disconnected.\n", lista_useri[user_index].identifier);
                                lista_useri[user_index].connected = 0;
                                close(fd);
                                for (int j = 0; j < num_fds; j++) {
                                    if (captured_fds[j].fd == fd) {
                                        captured_fds[j] = captured_fds[num_fds - 1];
                                        num_fds--;
                                        break;
                                    }
                                }}}}}}}}}