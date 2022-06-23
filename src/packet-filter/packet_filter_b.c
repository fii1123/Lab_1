#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <pthread.h>
#include <mqueue.h>

#include "errors.h"

#define MAX_PACKET_COUNT 65535
#define MSG_SIZE 20

struct filter {
    char *ip_from;
    char *ip_to;
    char *port_f;
    char *port_t;
};

// информация о пакетах в бинарном виде
struct packets {
    unsigned int p_info[MAX_PACKET_COUNT][5];
    unsigned short count;
    unsigned short total_len;
};

// общие данные для потоков
struct argum {
    int sock;
    char buff[4096];
    struct packets pack;
    struct filter *fl;
    mqd_t mq_id;
};

int filtration(struct filter *fl, struct iphdr *ip, struct udphdr *udp){

    unsigned int ip_verif;
    char ck = 0;
    if (fl->ip_from == NULL ||
            (inet_pton(AF_INET, fl->ip_from, &ip_verif) > 0 &&
            ip_verif == ip->saddr)){
        ck += 8;
    }
    if ((fl->port_f == NULL) ||
        (ntohs(udp->source) == atoi(fl->port_f))) {
        ck += 4;
    }
    if (fl->ip_to == NULL ||
            (inet_pton(AF_INET, fl->ip_to, &ip_verif) > 0 &&
            ip_verif == ip->daddr)){
        ck += 2;
    }
    if ((fl->port_t == NULL) ||
        (ntohs(udp->dest) == atoi(fl->port_t))) {
        ck += 1;
    }

    if (ck == 15) {
        return 1;
    }
    return 0;
}

void statistics(struct argum *a)
{
    // заголовки ip и фрейма
    #define FRAME_HSIZE 14
    #define IP_HSIZE (FRAME_HSIZE + sizeof(struct iphdr))

    struct iphdr *ip = (struct iphdr *)(a->buff + FRAME_HSIZE);
    struct udphdr *udp = (struct udphdr *)(a->buff + IP_HSIZE);

    // пакет udp
    if ((ip->protocol == 17) && (filtration(a->fl, ip, udp) == 1)) {
            // ip адреса
            a->pack.p_info[a->pack.count][0] = ip->saddr;
            a->pack.p_info[a->pack.count][1] = ip->daddr;

            //порты
            a->pack.p_info[a->pack.count][2] = ntohs(udp->source) << 16;
            a->pack.p_info[a->pack.count][2] += ntohs(udp->dest);

            unsigned short total_len = ntohs(ip->tot_len);

            //прочие данные
            a->pack.p_info[a->pack.count][3] = ip->protocol << 16;
            a->pack.p_info[a->pack.count][3] += total_len;
            a->pack.p_info[a->pack.count][4] = ntohs(udp->uh_ulen);
            a->pack.total_len += total_len;
            a->pack.count++;
    }
    // анализ завершен, можно писать данные далее
    a->buff[0] = 0;
}

void *listening_thread(void *argum)
{
    struct argum *a = argum;
    unsigned int buff_len = sizeof(a->buff);
    puts("Listening...\nPress \'q\' for exit");
    while (1) {
        if (a->buff[0] == 0 && recv(a->sock, a->buff, buff_len, 0) > 0) {
            statistics(a);
        }
    }
    pthread_exit(NULL);
}

void *secondary_tread(void *argum)
{
    struct argum *a = argum;
    struct packets p;

    char mq_buff[MSG_SIZE] = "1";
    struct mq_attr attr;

    mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);

    while (1) {
        mq_getattr(a->mq_id, &attr);

        if (attr.mq_curmsgs == 0) {
            // сигнал получен
            p = a->pack;
            sprintf(mq_buff, "%i", p.count);
            mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);

            unsigned short i;
            for (i = 0; i < p.count; i++) {
                memcpy(mq_buff, p.p_info[i], MSG_SIZE);
                mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);
            }
            sprintf(mq_buff, "%i", p.total_len);
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
        printf("packet-filter-b [interface name (enp*s* ...)]\n");
        exit(EXIT_FAILURE);
    }

    struct filter fl = {0};

    //создание фильтра
    int arg;
    for (arg = 1 ; arg <  argc - 1; arg++) {
        if (!strcmp(argv[arg], "-sa")) {
            fl.ip_from = argv[arg + 1];
        }
        if (!strcmp(argv[arg], "-sp")) {
            fl.port_f = argv[arg + 1];
        }
        if (!strcmp(argv[arg], "-da")) {
            fl.ip_to = argv[arg + 1];
        }
        if (!strcmp(argv[arg], "-dp")) {
            fl.port_t = argv[arg + 1];
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
        .mq_flags = 0,
        .mq_curmsgs = 0,
        .mq_maxmsg = 3000,
        .mq_msgsize = MSG_SIZE
    };
    mqd_t mq_id = mq_open("/packet-filter", O_CREAT | O_WRONLY,
                          S_IRUSR | S_IWUSR, &attr);

    // аргументы для передачи в поток
    struct argum a = {
        .buff = {0},
        .sock = raw_socket,
        .fl = &fl,
        .pack = {0},
        .mq_id = mq_id
    };

    // процессы
    pthread_create(&list_thread_id, &thr_attr, listening_thread, &a);
    pthread_create(&secd_thread_id, &thr_attr, secondary_tread, &a);

    while (getchar() != 'q') {
        if (a.pack.count == MAX_PACKET_COUNT -1) {
            memset(&a.pack, 0, sizeof (struct packets));
        }
    }

    mq_close(mq_id);
    mq_unlink("/packet-filter");
    exit(EXIT_SUCCESS);
}

