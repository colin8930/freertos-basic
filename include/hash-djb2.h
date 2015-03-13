#ifndef __HASH_DJB2_H__
#define __HASH_DJB2_H__

#include <unistd.h>
#include <stdint.h>
#define hash_init 5381
uint32_t hash_djb2(const uint8_t * str, ssize_t max);
uint32_t hash_path_djb2(const uint8_t * str, uint32_t hash);

#endif
