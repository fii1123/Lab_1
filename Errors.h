#ifndef ERRORS_H
#define ERRORS_H

#include <stdio.h>
#include <stdlib.h>
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

#include <mqueue.h>

int Raw_socket();

void Ioctl(int s, struct ifreq *if_req);

void Bind(const int s, struct sockaddr *sa, socklen_t len);

mqd_t Mq_open();

#endif // ERRORS_H
