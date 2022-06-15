#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mqueue.h>
#include <string.h>

struct Pocket_stat {
    char ip_from[16];
    char ip_to[16];
    char protocol;
    unsigned short port_f;
    unsigned short port_t;
    unsigned short total_len;
    unsigned short udp_len;
};

void Print_stat(struct Pocket_stat *Ps)
{
    printf("from [%s:%i]\tto [%s:%i]\n", Ps->ip_from, Ps->port_f, Ps->ip_to, Ps->port_t);
    printf("protocol: %i\tlen: %i\tudp len:%i\n",
           Ps->protocol, Ps->total_len, Ps->udp_len);
}

// дешифорвка
void Deshif(const unsigned int *buff, struct Pocket_stat *Ps)
{
    // ip
    inet_ntop(AF_INET, &buff[0], Ps->ip_from, 16);
    inet_ntop(AF_INET, &buff[1], Ps->ip_to, 16);

    // port
    Ps->port_f = (buff[2] & 0xFFFF0000) >> 16;
    Ps->port_t = buff[2] & 0xFFFF;

    // протокол, размер (полный и udp)
    Ps->protocol = (buff[3] & 0xFF0000) >> 16;
    Ps->total_len = buff[3] & 0xFFFF;
    Ps->udp_len = buff[4];
}

mqd_t Mq_open()
{
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_curmsgs = 0,
        .mq_maxmsg = 1000,
        .mq_msgsize = 20
    };
    mqd_t mq_id = mq_open("/LW", O_RDWR,
                          S_IRUSR | S_IWUSR, &attr);
    if (mq_id == -1) {
        perror("LW1 is not start!");
        exit(EXIT_FAILURE);
    }
    return mq_id;
}

struct Filtr {
    char *ip_from;
    char *ip_to;
    char *port_f;
    char *port_t;
};

int Check_filtr(struct Pocket_stat *Ps, struct Filtr *FL)
{
    char ck = 0;
    if ((FL->ip_from == NULL) ||
        (FL->ip_from != NULL && !strcmp(Ps->ip_from, FL->ip_from))) {
        ck += 8;
    }
    if ((FL->port_f == NULL) ||
        (FL->port_f != NULL && Ps->port_f == atoi(FL->port_f))) {
        ck += 4;
    }
    if ((FL->ip_to == NULL) ||
        (FL->ip_to != NULL && !strcmp(Ps->ip_to,FL->ip_to))) {
        ck += 2;
    }
    if ((FL->port_t == NULL) ||
        (FL->port_t != NULL && Ps->port_t == atoi(FL->port_t))) {
        ck += 1;
    }

    if (ck == 15) {
        return 1;
    }
    else {
        return 0;
    }
}

int main(int argc, char **argv)
{
    struct Filtr FL = {0};
    if ((argc % 2) == 0){
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
    }

    char buff[20] = {0};
    unsigned int total_size = 0;

    mqd_t mq_id = Mq_open();

    // Запрос статистики
    mq_send(mq_id, "1", 20, 1);

    // полчучение числа пакетов
    mq_receive(mq_id, buff, 20, NULL);
    unsigned short i, pockets_count = atoi(buff), filt_count = 0;

    struct Pocket_stat Ps;

    for (i = 0; i < pockets_count; i++){
        mq_receive(mq_id, buff, 20, NULL);
        Deshif((unsigned int *)buff, &Ps);

        if (Check_filtr(&Ps, &FL)) {
            Print_stat(&Ps);
            total_size += Ps.total_len;
            filt_count++;
        }
    }
    puts("==========================");
    printf("Pockets catch: %i\n", filt_count);
    printf("Total size: %i\n", total_size);

    mq_close(mq_id);

    return 0;
}
