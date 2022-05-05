#ifndef ROUTER_H
#define ROUTER_H

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TURN 20
#define SECOND 100000
#define INFINITY 20
#define REACHABLE   1
#define UNREACHABLE 0
#define DIRECTLY    1
#define INDIRECTLY  0

#define NETWORK(ip,mask) (ip & (~(UINT32_MAX >> mask)))
#define BROADCAST(ip,mask) (ip | (UINT32_MAX >> mask))

typedef struct {
  uint8_t mask;
  uint32_t ip;
} cidr_t;

typedef struct {
  uint32_t network_ip;
  uint32_t network_mask;
  uint32_t distance;
  uint8_t reachable;
} neighbour_t;

typedef struct {
  uint32_t  network_ip;
  uint8_t   network_mask;
  uint32_t  distance;
  uint32_t  path_ip;
  uint8_t   directness;
  uint8_t   last_update;
} route_t;

// router.c
void print_vec(route_t *vec, int v_len);

int32_t find_in_vec(route_t *vec, int v_len, uint32_t ip, uint8_t mask);

int32_t find_in_neigh(neighbour_t *vec, int v_len, uint32_t ip);

uint8_t own_ip(neighbour_t *vec, int v_len, uint32_t ip);


#endif
