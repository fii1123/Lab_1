#ifndef SAFE_FUNCTIONS_H
#define SAFE_FUNCTIONS_H

#include <stdio.h>
#include <stdlib.h>
// в данном файле содержатся функции с обработкой ошибок
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <net/if.h>

#include <features.h>           // Для версии glibc
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>       // Протоколы L2
#else
#include <asm/types.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>     // Протоколы L2
#endif

#include <pthread.h>
#include <mqueue.h>

int create_raw_socket();

void set_index_from_ireq(int s, struct ifreq *if_req);

void bind_socket(const int s, struct ifreq *if_req);

mqd_t msg_queue_open(struct mq_attr *attr);

#endif // SAFE_FUNCTIONS_H
