/* lib/crypt/xor.c - XOR AKA Authentication - 3GPP TS 34.108 */
#include "ogs-crypt.h"
#include <string.h>

static void rot_left(const uint8_t *in, uint8_t *out, int bits, int len)
{
    int i;
    int bytes = bits / 8;
    for (i = 0; i < len; i++)
        out[i] = in[(i + bytes) % len];
}

int ogs_xor_f2345(const uint8_t *k, const uint8_t *rand,
                  uint8_t *xres, uint8_t *ck,
                  uint8_t *ik,   uint8_t *ak)
{
    int i;
    uint8_t tmp[16];

    /* f2: XRES = K[0..7] XOR RAND[0..7] */
    for (i = 0; i < 8; i++)
        xres[i] = k[i] ^ rand[i];

    /* f3: CK = K rotateLeft(8) XOR RAND */
    rot_left(k, tmp, 8, 16);
    for (i = 0; i < 16; i++)
        ck[i] = tmp[i] ^ rand[i];

    /* f4: IK = K rotateLeft(16) XOR RAND */
    rot_left(k, tmp, 16, 16);
    for (i = 0; i < 16; i++)
        ik[i] = tmp[i] ^ rand[i];

    /* f5: AK = K[0..5] XOR RAND[0..5] */
    for (i = 0; i < 6; i++)
        ak[i] = k[i] ^ rand[i];

    return 0;
}

int ogs_xor_f1(const uint8_t *k,   const uint8_t *rand,
               const uint8_t *sqn, const uint8_t *amf,
               uint8_t *mac)
{
    int i;
    uint8_t tmp[16] = {0};
    memcpy(tmp,      sqn, 6);
    memcpy(tmp + 6,  amf, 2);
    memcpy(tmp + 8,  sqn, 6);
    memcpy(tmp + 14, amf, 2);
    for (i = 0; i < 16; i++)
        tmp[i] ^= k[i] ^ rand[i];
    memcpy(mac, tmp, 8);
    return 0;
}
