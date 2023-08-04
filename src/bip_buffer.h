#ifndef BIP_BUFFER_H
#define BIP_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct bip_buffer bip_buffer_t;

struct bip_buffer
{
    uint8_t *buffer;
    int ixa;
    int sza;
    int ixb;
    int szb;
    int buflen;
    int ix_reserve;
    int sz_reserve;

    int (*get_committed_size)(const bip_buffer_t *const bip_buff);
    int (*get_reservation_size)(const bip_buffer_t *const bip_buff);
    int (*get_buffer_size)(const bip_buffer_t *const bip_buff);
    uint8_t *(*reserve)(bip_buffer_t *bip_buff, size_t size, size_t *reserved);
    uint8_t *(*peek)(bip_buffer_t *bip_buff, size_t *size);
    void (*commit)(bip_buffer_t *bip_buff, size_t size);
    void (*consume)(bip_buffer_t *bip_buff, const size_t size);
    void (*clear)(bip_buffer_t *bip_buff);
};

bip_buffer_t *new_bip_buffer(size_t size);
void delete_bip_buffer(bip_buffer_t **const bip_buff);

#endif
