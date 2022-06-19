#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/in.h>
#include <pthread.h>
#include <mqueue.h>

#include "Errors.h"

#define MAX_POCKET_COUNT 65535
#define MSG_SIZE 20

struct Filtr {
    char *ip_from;
    char *ip_to;
    char *port_f;
    char *port_t;
};

// информация о пакетах в бинарном виде
struct Pockets {
    unsigned int p_info[MAX_POCKET_COUNT][5];
    unsigned short count;
    unsigned int total_len;
};

// общие данные для потоков
struct Argum {
    int sock;
    char buff[4096];
    struct Pockets pockets;
    struct Filtr *fl;
    mqd_t mq_id;
};

// сбор статистики
void Stat(struct Argum *a)
{
    // заголовки ip и фрейма
    #define FRAME_HSIZE 14
    #define IP_HSIZE (FRAME_HSIZE + sizeof(struct iphdr))

    struct iphdr *ip = (struct iphdr *)(a->buff + FRAME_HSIZE);
    struct udphdr *udp = (struct udphdr *)(a->buff + IP_HSIZE);

    // пакет udp
    if (ip->protocol == 17) {
        unsigned int ip_verif;

        char ck = 0;
        if (a->fl->ip_from == NULL ||
                (inet_pton(AF_INET, a->fl->ip_from, &ip_verif) > 0 &&
                ip_verif == ip->saddr)){
            ck += 8;
        }
        if ((a->fl->port_f == NULL) ||
            (ntohs(udp->source) == atoi(a->fl->port_f))) {
            ck += 4;
        }
        if (a->fl->ip_to == NULL ||
                (inet_pton(AF_INET, a->fl->ip_to, &ip_verif) > 0 &&
                ip_verif == ip->daddr)){
            ck += 2;
        }
        if ((a->fl->port_t == NULL) ||
            (ntohs(udp->dest) == atoi(a->fl->port_t))) {
            ck += 1;
        }

        if (ck == 15) {
            // ip адреса
            a->pockets.p_info[a->pockets.count][0] = ip->saddr;
            a->pockets.p_info[a->pockets.count][1] = ip->daddr;

            //порты
            a->pockets.p_info[a->pockets.count][2] = ntohs(udp->source << 16);
            a->pockets.p_info[a->pockets.count][2] += ntohs(udp->dest);

            //прочие данные
            unsigned short total_len = ntohs(ip->tot_len);
            a->pockets.p_info[a->pockets.count][3] = ip->protocol << 16;
            a->pockets.p_info[a->pockets.count][3] += total_len;
            a->pockets.p_info[a->pockets.count][4] = ntohs(udp->uh_ulen);
            a->pockets.count += total_len;
            a->pockets.count++;
        }
    }
    // анализ завершен, можно писать данные далее
    a->buff[0] = 0;
}

void *Listening_thread(void *argum)
{
    struct Argum *a = argum;
    unsigned int buff_len = sizeof(a->buff);
    puts("Listening...\nPress \'q\' for exit");
    while (1) {
        if (a->buff[0] == 0 && recv(a->sock, a->buff, buff_len, 0) > 0) {
            Stat(a);
        }
    }
    pthread_exit(NULL);
}

void *Secondary_tread(void *argum)
{
    struct Argum *a = argum;
    struct Pockets p;

    char mq_buff[MSG_SIZE] = "1";
    struct mq_attr attr;

    mq_send(a->mq_id, mq_buff, MSG_SIZE, 1);

    while (1) {
        mq_getattr(a->mq_id, &attr);

        if (attr.mq_curmsgs == 0) {
            // сигнал получен
            p = a->pockets;
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
        printf("LisIf [interface name (enp*s* ...)]\n");
        exit(EXIT_FAILURE);
    }

    struct Filtr FL = {0};

    //создание фильтра
    int arg;
    for (arg = 1 ; arg <  argc - 1; arg++) {
        if (!strcmp(argv[arg], "-sa")) {
            FL.ip_from = argv[arg + 1];
        }
        if (!strcmp(argv[arg], "-sp")) {
            FL.port_f = argv[arg + 1];
        }
        if (!strcmp(argv[arg], "-da")) {
            FL.ip_to = argv[arg + 1];
        }
        if (!strcmp(argv[arg], "-dp")) {
            FL.port_t = argv[arg + 1];
        }
    }

    // сама программа

    int raw_socket = Raw_socket();
    // определение индекса сетевого интерфейса
    struct ifreq if_req;
    strcpy(if_req.ifr_ifrn.ifrn_name, argv[1]);
    Ioctl(raw_socket, &if_req);

    Bind(raw_socket, &if_req);

    // подготовка потоков
    pthread_t List_thread_id, Secd_thread_id;
    pthread_attr_t thr_attr;
    pthread_attr_init(&thr_attr);

    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_curmsgs = 0,
        .mq_maxmsg = 3000,
        .mq_msgsize = MSG_SIZE
    };
    mqd_t mq_id = mq_open("/LisIf", O_CREAT | O_WRONLY,
                          S_IRUSR | S_IWUSR, &attr);

    // аргументы для передачи в поток
    struct Argum a = {
        .buff = {0},
        .sock = raw_socket,
        .fl = &FL,
        .pockets = {0},
        .mq_id = mq_id
    };

    // процессы
    pthread_create(&List_thread_id, &thr_attr, Listening_thread, &a);
    pthread_create(&Secd_thread_id, &thr_attr, Secondary_tread, &a);

    while (getchar() != 'q') {
        if (a.pockets.count == MAX_POCKET_COUNT -1) {
            memset(&a.pockets, 0, sizeof (struct Pockets));
        }
    }

    mq_close(mq_id);
    mq_unlink("/LisIf");
    exit(EXIT_SUCCESS);
}

