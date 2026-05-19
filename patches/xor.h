/* lib/crypt/xor.h - XOR AKA Authentication - 3GPP TS 34.108 */
#ifndef OGS_XOR_H
#define OGS_XOR_H

#include <stdint.h>

int ogs_xor_f2345(const uint8_t *k,   const uint8_t *rand,
                  uint8_t *xres, uint8_t *ck,
                  uint8_t *ik,   uint8_t *ak);

int ogs_xor_f1(const uint8_t *k,   const uint8_t *rand,
               const uint8_t *sqn, const uint8_t *amf,
               uint8_t *mac);

#endif /* OGS_XOR_H */
