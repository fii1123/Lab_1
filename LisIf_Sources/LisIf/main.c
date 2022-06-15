#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include "Errors.h"
#include "Pstat.h"

// общие данные для потоков
struct Argum {
    int sock;
    size_t bite_count;
    char buff[4096];
    struct Pockets_info pock_i;
    char stop;
};

// сбор статистики
void Stat(const char *buff, struct Pockets_info *pock_i)
{
    pock_i->p_info[pock_i->count] = Pocket_info(buff);
    pock_i->count++;
}

// ожидание сигнала и отправка сообщений
void *Sending(void *argum)
{
    struct Argum *a = argum;

    char mq_buff[20];

    mqd_t mq_id = Mq_open();
    mq_receive(mq_id, mq_buff, 20, NULL);
    // сигнал получен
    if (atoi(mq_buff) == 1) {
        sprintf(mq_buff,"%i",a->pock_i.count);
        mq_send(mq_id, mq_buff, 20, 1);
        unsigned short i;
        for (i = 0; i < a->pock_i.count; i++) {
            memcpy(mq_buff, a->pock_i.p_info[i], 20);
            mq_send(mq_id, mq_buff, 20, 1);
        }
    }
    mq_close(mq_id);
    a->stop = 1;
    pthread_exit(NULL);
}

void *Listening_thread(void *argum)
{
    struct Argum *a = argum;
    unsigned int buff_len = sizeof(a->buff);
    puts("Listening...\nPress \'q\' for exit");
    while (a->stop != 1) {
        a->bite_count = recv(a->sock, a->buff, buff_len, 0);
    }
    pthread_exit(NULL);
}

void *Secondary_tread(void *argum)
{
    struct Argum *a = argum;

    pthread_t Sending_thread;
    pthread_attr_t thr_attr;
    pthread_attr_init(&thr_attr);

    pthread_create(&Sending_thread, &thr_attr, Sending, argum);

    while (a->stop != 1) {
        if (a->bite_count > 0) {
            Stat(a->buff, &a->pock_i);
            a->bite_count = 0;
        }
    }
    pthread_exit(NULL);
}

int main (int argc, char **argv)
{
    if (argc < 2) {
        printf("LisIf [interface name (enp*s* ...)]\n");
        exit(EXIT_FAILURE);
    }

    int raw_socket = Raw_socket();

    struct ifreq if_req;

    strcpy(if_req.ifr_ifrn.ifrn_name, argv[1]);

    // определение индекса сетевого интерфейса
    Ioctl(raw_socket, &if_req);

    struct sockaddr_ll sa = {
        .sll_family = AF_PACKET,
        .sll_protocol = htons(ETH_P_ALL),
        .sll_ifindex = if_req.ifr_ifindex
    };

    Bind(raw_socket, (struct sockaddr *) &sa, sizeof(sa));

    // подготовка потоков
    pthread_t List_thread_id, Secd_thread_id;
    pthread_attr_t thr_attr;
    pthread_attr_init(&thr_attr);

    // аргументы для передачи в поток
    struct Argum a = {
        .sock = raw_socket,
        .pock_i = {0},
        .stop = 0
    };

    // процессы
    pthread_create(&List_thread_id, &thr_attr, Listening_thread, &a);
    pthread_create(&Secd_thread_id, &thr_attr, Secondary_tread, &a);

    while (a.stop != 1) {}

    pthread_join(List_thread_id, NULL);
    pthread_join(Secd_thread_id, NULL);

    Free_Pocket_info(&a.pock_i);
    mq_unlink("/LisIf");
    exit(EXIT_SUCCESS);
}

