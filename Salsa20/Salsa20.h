#pragma once

#include <tuple>
#include <cstdint>

class Salsa20 {
public:
    static const size_t CHUNK_SIZE = 64;

    static void crypt16(
            const uint8_t key[16],
            uint8_t nonce[8],
            uint8_t chunk[CHUNK_SIZE]
    );

    static void crypt32(
            const uint8_t key[32],
            uint8_t nonce[8],
            uint8_t chunk[CHUNK_SIZE]
    );
private:
    Salsa20() = default;
    Salsa20(const Salsa20& salsa20);
    Salsa20& operator =(const Salsa20& salsa20);

    static void quarterround(
            uint32_t& y0,
            uint32_t& y1,
            uint32_t& y2,
            uint32_t& y3
    );
    static void rowround(uint32_t y[16]);
    static void columnround(uint32_t x[16]);
    static void doubleround(uint32_t y[16]);
    static uint32_t littleendian(const uint8_t b[4]);
    static uint32_t un_littleendian(uint8_t b[4], uint32_t w);
    static void hash(uint8_t sequence[64]);
    static void expand(
            const uint8_t k0[16],
            const uint8_t *k1,
            uint8_t n[16],
            uint8_t sequence[64]);
    static void crypt(
            const uint8_t k0[16],
            const uint8_t *k1,
            uint8_t nonce[8],
            uint8_t chunk[CHUNK_SIZE]
    );


    static uint32_t rotate(uint32_t value, uint8_t shift);

    static constexpr uint8_t tau[4][4] = {
            {101, 120, 112,  97},
            {110, 100,  32,  49},
            { 54,  45,  98, 121},
            {116, 101, 32, 107}
    };

    static constexpr uint8_t omega[4][4] = {
            {101, 120, 112,  97},
            {110, 100,  32,  51},
            { 50,  45,  98, 121},
            {116, 101,  32, 107}
    };
};