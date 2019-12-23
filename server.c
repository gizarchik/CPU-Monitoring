#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

volatile int must_exit = 0;
volatile int flag = 0;
volatile int server_socket;

void handle_sigterm_and_sigint(int signum) {
    must_exit = 1;
    if (flag == 1) {
        close(server_socket);
        exit(1);
    }
}

struct Client {
    char* name;
    int32_t cpu_load_percents;
};

int InitAndGetServerSocket(uint16_t hostshort_port) {
    struct sockaddr_in full_addr;
    full_addr.sin_family = AF_INET;
    full_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    full_addr.sin_port = htons(hostshort_port);

    int socket_server = socket(AF_INET, SOCK_DGRAM, 0);
    if (bind(socket_server, (struct sockaddr*) &full_addr, sizeof(struct sockaddr_in))) {
        perror("bind");
        close(socket_server);
        exit(1);
    }

    struct sigaction action_term_and_int;
    memset(&action_term_and_int, 0, sizeof(action_term_and_int));
    action_term_and_int.sa_handler = handle_sigterm_and_sigint;
    action_term_and_int.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &action_term_and_int, NULL);
    sigaction(SIGINT, &action_term_and_int, NULL);

    return socket_server;
}

void PrintDataBase(struct Client* client_data_base, size_t size) {
    for (size_t i = 0; i < size; ++i) {
        printf("Client %s has CPU load: %d%%\n",
                client_data_base[i].name,
                client_data_base[i].cpu_load_percents);
    }
}

int main(int argc, char* argv[]) {
    uint16_t port = strtol(argv[1], NULL, 10);
    time_t period = strtol(argv[2], NULL, 10);

    server_socket = InitAndGetServerSocket(port);

    struct Client* clients_db;
    const size_t CAPACITY_DB = 1000;
    clients_db = malloc(CAPACITY_DB * sizeof(struct Client));
    size_t num_clients = 0;

    while (1) {
        time_t start_time;
        time(&start_time);
        time_t current_time = start_time;
        int32_t remain = period - (current_time - start_time);
        while (remain > 0) {
            struct Client new_client;
            struct timeval timeout;
            timeout.tv_sec = remain;
            timeout.tv_usec = 0;
            setsockopt(server_socket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
            flag = 1;
            char recv_msg[256];
            if (0 < recvfrom(server_socket,
                         recv_msg,
                         sizeof(recv_msg),
                         0,
                         NULL,
                         NULL)) {
                sscanf(recv_msg, "%s %d\n", new_client.name, &new_client.cpu_load_percents);
                clients_db[num_clients] = new_client;
                ++num_clients;
            }
            flag = 0;
            time(&current_time);
            remain = period - (current_time - start_time);
            if (must_exit == 1) {
                break;
            }
        }
        if (must_exit == 1) {
            break;
        }
        PrintDataBase(clients_db, num_clients);
        num_clients = 0;
    }

    close(server_socket);
    return 0;
}