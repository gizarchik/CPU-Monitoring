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

void Init(struct sockaddr_in* send_addr, int* sock, char* address, uint16_t hostshort_port) {
    send_addr->sin_family = AF_INET;
    send_addr->sin_addr.s_addr = inet_addr(address);
    send_addr->sin_port = htons(hostshort_port);

    *sock = socket(AF_INET, SOCK_DGRAM, 0);

    struct sigaction action_term_and_int;
    memset(&action_term_and_int, 0, sizeof(action_term_and_int));
    action_term_and_int.sa_handler = handle_sigterm_and_sigint;
    action_term_and_int.sa_flags = SA_RESTART;
    sigaction(SIGTERM, &action_term_and_int, NULL);
    sigaction(SIGINT, &action_term_and_int, NULL);
}

int32_t GetCPULoad() {
    struct ProcStat {
        int64_t user;
        int64_t nice;
        int64_t system;
        int64_t idle;
    };

    struct ProcStat first, second;
    FILE* proc_stat = fopen("/proc/stat", "r");
    fscanf(proc_stat, "%*s %ld %ld %ld %ld", &first.user, &first.nice,
            &first.system, &first.idle);

    sleep(1);

    fscanf(proc_stat, "%*s %ld %ld %ld %ld", &second.user, &second.nice,
            &second.system, &second.idle);

    int64_t sum_exec_proc_first = first.user + first.nice + first.system;
    int64_t sum_exec_proc_second = second.user + second.nice + second.system;
    int64_t sum_all_proc_first = sum_exec_proc_first + first.idle;
    int64_t sum_all_proc_second = sum_exec_proc_second + second.idle;
    //printf("%ld %ld %ld %ld\n", sum_exec_proc_first, sum_exec_proc_second, sum_all_proc_first, sum_all_proc_second);
    int32_t cpu_load_avg = (sum_exec_proc_second - sum_exec_proc_first) * 100 /
                           (sum_all_proc_second - sum_all_proc_first);


    //printf("%d\n", cpu_load_avg);
    fclose(proc_stat);
    return cpu_load_avg;
}

int main(int argc, char* argv[]) {
    char* address = argv[1];
    uint16_t port = strtol(argv[2], NULL, 10);
    char* client_name = argv[3];
    time_t period = strtol(argv[4], NULL, 10);

    struct sockaddr_in send_addr;
    int sock;
    Init(&send_addr, &sock, address, port);

    while(1) {
        time_t start_time;
        time(&start_time);
        int32_t cpu_load = GetCPULoad();
        char send_msg[256];
        snprintf(send_msg, sizeof(send_msg), "%s %d\n", client_name, cpu_load);
        //printf("%s", send_msg);
        sendto(sock,
               send_msg,
               strlen(send_msg),
               0,
               (const struct sockaddr*) &send_addr,
               sizeof(send_addr));
        if (must_exit == 1) {
            break;
        }
        time_t end_time;
        time(&end_time);
        sleep(period - (end_time - start_time));
    }

    return 0;
}
