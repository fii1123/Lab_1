#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/in.h>

#include "safe_functions.h"

// заголовки ip и фрейма
#define FRAME_HSIZE 14
#define IP_HSIZE (FRAME_HSIZE + sizeof(struct iphdr))

// обычно, максимальный размер ip пакета до 576
#define BUFF_SIZE 576

// размер сообщения = 4 + 2 байта
#define MSG_SIZE 6

struct filter {
    char *ip_from;
    char *ip_to;
    char *port_f;
    char *port_t;
};

// информация о пакетах в бинарном виде
struct statistics {
    unsigned int bytes_count;
    unsigned short packets_count;
};

// общие данные для потоков
struct argum {
    int sock;
    char buff[BUFF_SIZE];
    int verif;
    struct iphdr *ip;
    struct udphdr *udp;
    struct statistics stat;
    struct filter *fl;
    mqd_t mq_id;
};

int filtration(struct filter *fl, struct iphdr *ip, struct udphdr *udp){

    unsigned int ip_verif;
    if (fl->ip_from != NULL &&
            (inet_pton(AF_INET, fl->ip_from, &ip_verif) &&
            ip_verif != ip->saddr)){
        return 0;
    }
    if ((fl->port_f != NULL) &&
        (ntohs(udp->source) != atoi(fl->port_f))) {
        return 0;
    }
    if (fl->ip_to != NULL &&
            (inet_pton(AF_INET, fl->ip_to, &ip_verif) &&
            ip_verif != ip->daddr)){
        return 0;
    }
    if (fl->port_t != NULL &&
        (ntohs(udp->dest) != atoi(fl->port_t))) {
        return 0;
    }
    return 1;
}

void create_statistics(struct statistics *stat, struct iphdr *ip){
    stat->bytes_count += ntohs(ip->tot_len) + FRAME_HSIZE;
    stat->packets_count++;
}

void *listening_thread(void *argum)
{
    struct argum *a = argum;

    unsigned int buff_len = sizeof(a->buff);
    unsigned short packet_bytes;

    puts("Listening...\nPress \'q\' for exit");
    while (1) {
        // verif отвечает как за соответствие пакета,
        // так и за разрешение читать далее
        if (a->verif == 0) {
            packet_bytes = recv(a->sock, a->buff, buff_len, 0);

            // пакет udp + фильтр
            a->verif = (packet_bytes > 0 && (a->ip->protocol == 17)
                    && (filtration(a->fl, a->ip, a->udp) == 1));
        }
    }
    pthread_exit(NULL);
}

void *secondary_tread(void *argum)
{
    struct argum *a = argum;

    char mq_buff[MSG_SIZE] = "1";
    struct mq_attr attr;

    mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);

    while (1) {

        if (a->verif == 1) {
            create_statistics(&a->stat, a->ip);
            a->verif = 0;
        }

        mq_getattr(a->mq_id, &attr);
        if (attr.mq_curmsgs == 0) {
            // сигнал получен

            sprintf(mq_buff, "%i", a->stat.packets_count);
            mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);

            sprintf(mq_buff, "%i", a->stat.bytes_count);
            mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);

            // снова ожидание
            memcpy(mq_buff, "1", MSG_SIZE);
            mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);
        }
    }
}

int main (int argc, char **argv)
{
    if (argc < 2) {
        printf("packet-filter [interface name (enp*s* ...)]\n");
        exit(EXIT_FAILURE);
    }

    struct filter fl = {0};

    //создание фильтра
    int arg, ip_verif;
    for (arg = 1; arg <  argc - 1; arg++) {
        if (!strcmp(argv[arg], "-sa")) {
            if (inet_pton(AF_INET, argv[arg + 1], &ip_verif) > 0) {
                fl.ip_from = argv[arg + 1];
            }
            else {
                perror("source address error!");
                exit(EXIT_FAILURE);
            }
        }
        if (!strcmp(argv[arg], "-sp")) {
            if (atoi(argv[arg + 1]) < 0 ||
                    atoi(argv[arg + 1]) > UINT16_MAX) {
                printf("source port error!");
                exit(EXIT_FAILURE);
            }
            else {
                fl.port_f = argv[arg + 1];
            }
        }
        if (!strcmp(argv[arg], "-da")) {
            if (inet_pton(AF_INET, argv[arg + 1], &ip_verif) > 0) {
                fl.ip_from = argv[arg + 1];
            }
            else {
                perror("distination address error!");
                exit(EXIT_FAILURE);
            }
        }
        if (!strcmp(argv[arg], "-dp")) {
            if (atoi(argv[arg + 1]) < 0 ||
                    atoi(argv[arg + 1]) > UINT16_MAX) {
                printf("distination port error!");
                exit(EXIT_FAILURE);
            }
            else {
                fl.port_f = argv[arg + 1];
            }
        }
    }

    // сама программа

    int raw_socket = create_raw_socket();

    // определение индекса сетевого интерфейса
    struct ifreq if_req;
    strcpy(if_req.ifr_ifrn.ifrn_name, argv[1]);
    set_index_from_ireq(raw_socket, &if_req);

    bind_socket(raw_socket, &if_req);

    // подготовка потоков
    pthread_t list_thread_id, secd_thread_id;
    pthread_attr_t thr_attr;
    pthread_attr_init(&thr_attr);

    struct mq_attr attr = {
        .mq_maxmsg = 10,
        .mq_msgsize = MSG_SIZE
    };

    mqd_t mq_id = msg_queue_open(&attr);

    // аргументы для передачи в поток

    struct argum a = {
        .buff = {0},
        .ip = (struct iphdr *)(a.buff + FRAME_HSIZE),
        .udp = (struct udphdr *)(a.buff + IP_HSIZE),
        .sock = raw_socket,
        .verif = 0,
        .fl = &fl,
        .stat = {0},
        .mq_id = mq_id
    };

    // процессы
    pthread_create(&list_thread_id, &thr_attr, listening_thread, &a);
    pthread_create(&secd_thread_id, &thr_attr, secondary_tread, &a);

    while (getchar() != 'q') {

    }

    mq_close(mq_id);
    mq_unlink("/packet-filter");
    pthread_cancel(list_thread_id);
    pthread_cancel(secd_thread_id);
    close(raw_socket);
    exit(EXIT_SUCCESS);
}

