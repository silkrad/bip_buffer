#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "bip_buffer.h"

static int bb_get_committed_size(const bip_buffer_t *const bip_buff);
static int bb_get_reservation_size(const bip_buffer_t *const bip_buff);
static int bb_get_buffer_size(const bip_buffer_t *const bip_buff);
static int bb_get_space_after_a(const bip_buffer_t *const bip_buff);
static int bb_get_b_free_space(const bip_buffer_t *const bip_buff);
static uint8_t *bb_reserve(bip_buffer_t *bip_buff, size_t size, size_t *reserved);
static uint8_t *bb_peek(bip_buffer_t *bip_buff, size_t *size);
static void bb_commit(bip_buffer_t *bip_buff, size_t size);
static void bb_consume(bip_buffer_t *bip_buff, const size_t size);
static void bb_clear(bip_buffer_t *bip_buff);
static void bb_free_buffer(bip_buffer_t *bip_buff);

bip_buffer_t *new_bip_buffer(const size_t size)
{
    bip_buffer_t *bip_buff = NULL;

    assert(0 < size);

    if (NULL == (bip_buff = calloc(sizeof *bip_buff, 1)))
    {
        fprintf(stderr, "bip_buff calloc error");
    }
    else if (NULL == (bip_buff->buffer = calloc(size, 1)))
    {
        fprintf(stderr, "bip_buff->buffer calloc error");
    }
    else
    {
        bip_buff->buflen = size;
        bip_buff->ixa = 0;
        bip_buff->sza = 0;
        bip_buff->ixb = 0;
        bip_buff->szb = 0;
        bip_buff->ix_reserve = 0;
        bip_buff->sz_reserve = 0;

        bip_buff->commit = bb_commit;
        bip_buff->get_committed_size = bb_get_committed_size;
        bip_buff->get_reservation_size = bb_get_reservation_size;
        bip_buff->get_buffer_size = bb_get_buffer_size;
        bip_buff->reserve = bb_reserve;
        bip_buff->peek = bb_peek;
        bip_buff->consume = bb_consume;
        bip_buff->clear = bb_clear;
    }

    return bip_buff;
}

void delete_bip_buffer(bip_buffer_t **const bip_buff)
{
    assert(NULL != bip_buff);
    
    if (NULL != *bip_buff)
    {
        bb_free_buffer(*bip_buff);
        *bip_buff = NULL;
    }
}

static int bb_get_committed_size(const bip_buffer_t *const bip_buff)
{
    assert(NULL != bip_buff);
    return bip_buff->sza + bip_buff->szb;
}

static int bb_get_reservation_size(const bip_buffer_t *const bip_buff)
{
    assert(NULL != bip_buff);
    return bip_buff->sz_reserve;
}

static int bb_get_buffer_size(const bip_buffer_t *const bip_buff)
{
    assert(NULL != bip_buff);
    return bip_buff->buflen;
}

static int bb_get_space_after_a(const bip_buffer_t *const bip_buff)
{
    assert(NULL != bip_buff);
    return bip_buff->buflen - bip_buff->ixa - bip_buff->sza;
}

static int bb_get_b_free_space(const bip_buffer_t *const bip_buff)
{
    assert(NULL != bip_buff);
    return bip_buff->ixa - bip_buff->ixb - bip_buff->szb;
}

static uint8_t *bb_reserve(bip_buffer_t *bip_buff, size_t size, size_t *reserved)
{
    uint8_t *buff_ptr = NULL;

    assert(NULL != bip_buff);
    assert(NULL != reserved);

    *reserved = 0;

    if (bip_buff->szb)
    {
        size_t free_space = bb_get_b_free_space(bip_buff);

        if (size < free_space)
        {
            free_space = size;
        }
        
        if (0 < free_space)
        {
            bip_buff->sz_reserve = free_space;
            bip_buff->ix_reserve = bip_buff->ixb + bip_buff->szb;
            buff_ptr = bip_buff->buffer + bip_buff->ix_reserve;
            *reserved = free_space;
        }
    }
    else
    {
        size_t free_space = bb_get_space_after_a(bip_buff);

        if (free_space >= bip_buff->ixa && 0 < free_space)
        {
            if (size < free_space)
            {
                free_space = size;
            }

            bip_buff->sz_reserve = free_space;
            bip_buff->ix_reserve = bip_buff->ixa + bip_buff->sza;
            buff_ptr = bip_buff->buffer + bip_buff->ix_reserve;
            *reserved = free_space;
        }
        else if (0 < bip_buff->ixa)
        {
            if (bip_buff->ixa < size)
            {
                size = bip_buff->ixa;
            }

            bip_buff->sz_reserve = size;
            bip_buff->ix_reserve = 0;
            buff_ptr = bip_buff->buffer;
            *reserved = size;
        }
    }

    return buff_ptr;
}

static uint8_t *bb_peek(bip_buffer_t *bip_buff, size_t *size)
{
    uint8_t *data = NULL;

    assert(NULL != bip_buff);

    if (0 == bip_buff->sza)
    {
        *size = 0;
    }
    else
    {
        *size = bip_buff->sza;
        data = bip_buff->buffer + bip_buff->ixa;
    }

    return data;
}

static void bb_commit(bip_buffer_t *bip_buff, size_t size)
{
    assert(NULL != bip_buff);
    assert(size <= bip_buff->sz_reserve);

    if (size == 0)
    {
        bip_buff->sz_reserve = 0;
        bip_buff->ix_reserve = 0;
    }
    else
    {
        if (size > bip_buff->sz_reserve)
        {
            size = bip_buff->sz_reserve;
        }

        if (0 == bip_buff->sza && 0 == bip_buff->szb)
        {
            bip_buff->ixa = bip_buff->ix_reserve;
            bip_buff->sza = size;
        }
        else if (bip_buff->ix_reserve == bip_buff->sza + bip_buff->ixa)
        {
            bip_buff->sza += size;
        }
        else
        {
            bip_buff->szb += size;
        }

        bip_buff->ix_reserve = 0;
        bip_buff->sz_reserve = 0;
    }
}

static void bb_consume(bip_buffer_t *bip_buff, const size_t size)
{
    assert(NULL != bip_buff);

    if (size >= bip_buff->sza)
    {
        bip_buff->ixa = bip_buff->ixb;
        bip_buff->sza = bip_buff->szb;
        bip_buff->ixb = 0;
        bip_buff->szb = 0;
    }
    else
    {
        bip_buff->sza -= size;
        bip_buff->ixa += size;
    }
}

static void bb_clear(bip_buffer_t *bip_buff)
{
    assert(NULL != bip_buff);

    bip_buff->ixa = 0;
    bip_buff->sza = 0;
    bip_buff->ixb = 0;
    bip_buff->szb = 0;
    bip_buff->ix_reserve = 0;
    bip_buff->sz_reserve = 0;
}

static void bb_free_buffer(bip_buffer_t *bip_buff)
{
    assert(NULL != bip_buff);

    if (NULL != bip_buff->buffer)
    {
        bb_clear(bip_buff);
        free(bip_buff->buffer);
        bip_buff->buffer = NULL;
        bip_buff->buflen = 0;
    }
}
