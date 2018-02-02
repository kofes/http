#include "Salsa20.h"

constexpr uint8_t Salsa20::tau[4][4];
constexpr uint8_t Salsa20::omega[4][4];

uint32_t Salsa20::rotate(uint32_t value, uint8_t shift) {
    return (value << shift) | (value >> (32 - shift));
}

void Salsa20::quarterround(
        std::uint32_t& y0,
        std::uint32_t& y1,
        std::uint32_t& y2,
        std::uint32_t& y3
) {
    y1 ^= rotate((y0 + y3), 7);
    y2 ^= rotate((y1 + y0), 9);
    y3 ^= rotate((y2 + y1), 13);
    y0 ^= rotate((y3 + y2), 18);
};

void Salsa20::rowround(std::uint32_t y[16]) {
    quarterround(y[0], y[1], y[2], y[3]);
    quarterround(y[5], y[6], y[7], y[4]);
    quarterround(y[10], y[11], y[8], y[9]);
    quarterround(y[15], y[12], y[13], y[14]);
}

void Salsa20::columnround(std::uint32_t y[16]) {
    quarterround(y[0], y[4], y[8], y[12]);
    quarterround(y[5], y[9], y[13], y[1]);
    quarterround(y[10], y[14], y[2], y[6]);
    quarterround(y[15], y[3], y[7], y[11]);
}

void Salsa20::doubleround(std::uint32_t y[16]) {
    columnround(y);
    rowround(y);
}

uint32_t Salsa20::littleendian(const std::uint8_t b[4]) {
    uint32_t result;
    result = b[3];
    result = (result << 8) + b[2];
    result = (result << 8) + b[1];
    result = (result << 8) + b[0];
    return result;
}

uint32_t Salsa20::un_littleendian(uint8_t b[4], uint32_t w) {
    b[0] = w;
    b[1] = w >> 8;
    b[2] = w >> 16;
    b[3] = w >> 24;
}

void Salsa20::hash(uint8_t sequence[64]) {
    uint32_t x[16];
    uint32_t z[16];

    for (std::uint8_t i = 0; i < 16; ++i)
        x[i] = z[i] = littleendian(sequence + (4 * i));
    for (std::uint8_t i = 0; i < 10; ++i)
        doubleround(z);
    for (std::uint8_t i = 0; i < 16; ++i)
        un_littleendian(sequence + (4 * i), z[i] + x[i]);
}


void Salsa20::expand(
        const uint8_t k0[16],
        const uint8_t *k1,
        uint8_t n[16],
        uint8_t sequence[64]) {
    const uint8_t (*cnst)[4] = omega;

    if (k1 == nullptr) {
        k1 = k0;
        cnst = tau;
    }

    std::copy(cnst[0], cnst[0] + 4, sequence);     //4
    std::copy(k0, k0 + 16, sequence + 4);          //20
    std::copy(cnst[1], cnst[1] + 4, sequence + 20);//24
    std::copy(n, n + 16, sequence + 24);           //40
    std::copy(cnst[2], cnst[2] + 4, sequence + 40);//44
    std::copy(k1, k1 + 16, sequence + 44);         //60
    std::copy(cnst[3], cnst[3] + 4, sequence + 60);//64

    hash(sequence);
}

void Salsa20::crypt16(
        const uint8_t key[16],
        uint8_t nonce[8],
        uint8_t chunk[CHUNK_SIZE]
) {
    crypt(
            key,
            nullptr,
            nonce,
            chunk
    );
}
void Salsa20::crypt32(
        const uint8_t key[32],
        uint8_t nonce[8],
        uint8_t chunk[CHUNK_SIZE]
) {
    crypt(
            key,
            key+16,
            nonce,
            chunk
    );
}

void Salsa20::crypt(
        const uint8_t k0[16],
        const uint8_t *k1,
        uint8_t nonce[8],
        uint8_t chunk[CHUNK_SIZE]
) {
    uint8_t hash_seq[64];
    uint8_t n[16] = {0};

    for (uint8_t i = 0; i < 8; ++i)
        n[i] = nonce[i];

    un_littleendian(n + 8, 0);
    expand(k0, k1, n, hash_seq);

    for (size_t i = 0; i < CHUNK_SIZE; ++i)
        chunk[i] ^= hash_seq[i];
}