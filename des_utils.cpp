#include "des_utils.h"

#include <cctype>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

const int IP[64] = {
    58, 50, 42, 34, 26, 18, 10, 2, 60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6, 64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17, 9, 1, 59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5, 63, 55, 47, 39, 31, 23, 15, 7};

const int IP_1[64] = {
    40, 8, 48, 16, 56, 24, 64, 32, 39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30, 37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28, 35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26, 33, 1, 41, 9, 49, 17, 57, 25};

const int E[48] = {
    32, 1, 2, 3, 4, 5, 4, 5, 6, 7, 8, 9, 8, 9, 10, 11,
    12, 13, 12, 13, 14, 15, 16, 17, 16, 17, 18, 19, 20, 21, 20, 21,
    22, 23, 24, 25, 24, 25, 26, 27, 28, 29, 28, 29, 30, 31, 32, 1};

const int P[32] = {
    16, 7, 20, 21, 29, 12, 28, 17,
    1, 15, 23, 26, 5, 18, 31, 10,
    2, 8, 24, 14, 32, 27, 3, 9,
    19, 13, 30, 6, 22, 11, 4, 25};

const int PC1[56] = {
    57, 49, 41, 33, 25, 17, 9,
    1, 58, 50, 42, 34, 26, 18,
    10, 2, 59, 51, 43, 35, 27,
    19, 11, 3, 60, 52, 44, 36,
    63, 55, 47, 39, 31, 23, 15,
    7, 62, 54, 46, 38, 30, 22,
    14, 6, 61, 53, 45, 37, 29,
    21, 13, 5, 28, 20, 12, 4};

const int PC2[48] = {
    14, 17, 11, 24, 1, 5, 3, 28,
    15, 6, 21, 10, 23, 19, 12, 4,
    26, 8, 16, 7, 27, 20, 13, 2,
    41, 52, 31, 37, 47, 55, 30, 40,
    51, 45, 33, 48, 44, 49, 39, 56,
    34, 53, 46, 42, 50, 36, 29, 32};

const int SHIFTS[16] = {1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1};

const int SBOX[8][4][16] = {
    {{14,4,13,1,2,15,11,8,3,10,6,12,5,9,0,7},
     {0,15,7,4,14,2,13,1,10,6,12,11,9,5,3,8},
     {4,1,14,8,13,6,2,11,15,12,9,7,3,10,5,0},
     {15,12,8,2,4,9,1,7,5,11,3,14,10,0,6,13}},

    {{15,1,8,14,6,11,3,4,9,7,2,13,12,0,5,10},
     {3,13,4,7,15,2,8,14,12,0,1,10,6,9,11,5},
     {0,14,7,11,10,4,13,1,5,8,12,6,9,3,2,15},
     {13,8,10,1,3,15,4,2,11,6,7,12,0,5,14,9}},

    {{10,0,9,14,6,3,15,5,1,13,12,7,11,4,2,8},
     {13,7,0,9,3,4,6,10,2,8,5,14,12,11,15,1},
     {13,6,4,9,8,15,3,0,11,1,2,12,5,10,14,7},
     {1,10,13,0,6,9,8,7,4,15,14,3,11,5,2,12}},

    {{7,13,14,3,0,6,9,10,1,2,8,5,11,12,4,15},
     {13,8,11,5,6,15,0,3,4,7,2,12,1,10,14,9},
     {10,6,9,0,12,11,7,13,15,1,3,14,5,2,8,4},
     {3,15,0,6,10,1,13,8,9,4,5,11,12,7,2,14}},

    {{2,12,4,1,7,10,11,6,8,5,3,15,13,0,14,9},
     {14,11,2,12,4,7,13,1,5,0,15,10,3,9,8,6},
     {4,2,1,11,10,13,7,8,15,9,12,5,6,3,0,14},
     {11,8,12,7,1,14,2,13,6,15,0,9,10,4,5,3}},

    {{12,1,10,15,9,2,6,8,0,13,3,4,14,7,5,11},
     {10,15,4,2,7,12,9,5,6,1,13,14,0,11,3,8},
     {9,14,15,5,2,8,12,3,7,0,4,10,1,13,11,6},
     {4,3,2,12,9,5,15,10,11,14,1,7,6,0,8,13}},

    {{4,11,2,14,15,0,8,13,3,12,9,7,5,10,6,1},
     {13,0,11,7,4,9,1,10,14,3,5,12,2,15,8,6},
     {1,4,11,13,12,3,7,14,10,15,6,8,0,5,9,2},
     {6,11,13,8,1,4,10,7,9,5,0,15,14,2,3,12}},

    {{13,2,8,4,6,15,11,1,10,9,3,14,5,0,12,7},
     {1,15,13,8,10,3,7,4,12,5,6,11,0,14,9,2},
     {7,11,4,1,9,12,14,2,0,6,10,13,15,3,5,8},
     {2,1,14,7,4,10,8,13,15,12,9,0,3,5,6,11}}
};

char ToHex(unsigned char v) {
    return (v < 10) ? static_cast<char>('0' + v) : static_cast<char>('A' + (v - 10));
}

unsigned char HexVal(char c) {
    if (c >= '0' && c <= '9') return static_cast<unsigned char>(c - '0');
    if (c >= 'a' && c <= 'f') return static_cast<unsigned char>(c - 'a' + 10);
    if (c >= 'A' && c <= 'F') return static_cast<unsigned char>(c - 'A' + 10);
    throw std::runtime_error("invalid hex char");
}

}  // namespace

DesCipher::DesCipher(const std::string& key_text) {
    std::memset(key_, 0, sizeof(key_));
    for (size_t i = 0; i < sizeof(key_) && i < key_text.size(); ++i) {
        key_[i] = static_cast<unsigned char>(key_text[i]);
    }
}

void DesCipher::BytesToBits(const unsigned char* in, unsigned char* out, int n) {
    for (int i = 0; i < n; ++i) {
        out[i] = static_cast<unsigned char>((in[i / 8] >> (7 - (i % 8))) & 0x01);
    }
}

void DesCipher::BitsToBytes(const unsigned char* in, unsigned char* out, int n) {
    std::memset(out, 0, (n + 7) / 8);
    for (int i = 0; i < n; ++i) {
        out[i / 8] |= static_cast<unsigned char>(in[i] << (7 - (i % 8)));
    }
}

void DesCipher::Permute(const unsigned char* in, unsigned char* out, const int* table, int n) {
    std::vector<unsigned char> in_bits(n > 56 ? 64 : n, 0);
    BytesToBits(in, in_bits.data(), static_cast<int>(in_bits.size()));

    std::vector<unsigned char> out_bits(n, 0);
    for (int i = 0; i < n; ++i) {
        out_bits[i] = in_bits[table[i] - 1];
    }
    BitsToBytes(out_bits.data(), out, n);
}

void DesCipher::Xor(unsigned char* a, const unsigned char* b, int n) {
    for (int i = 0; i < n; ++i) {
        a[i] ^= b[i];
    }
}

void DesCipher::GenerateSubKeys() const {
    if (!subkeys_.empty()) {
        return;
    }

    unsigned char key56[7] = {0};
    Permute(key_, key56, PC1, 56);

    unsigned char key56_bits[56] = {0};
    BytesToBits(key56, key56_bits, 56);

    unsigned char c[28] = {0};
    unsigned char d[28] = {0};
    for (int i = 0; i < 28; ++i) {
        c[i] = key56_bits[i];
        d[i] = key56_bits[i + 28];
    }

    for (int round = 0; round < 16; ++round) {
        for (int s = 0; s < SHIFTS[round]; ++s) {
            unsigned char c0 = c[0];
            unsigned char d0 = d[0];
            for (int i = 0; i < 27; ++i) {
                c[i] = c[i + 1];
                d[i] = d[i + 1];
            }
            c[27] = c0;
            d[27] = d0;
        }

        unsigned char cd[56] = {0};
        for (int i = 0; i < 28; ++i) {
            cd[i] = c[i];
            cd[i + 28] = d[i];
        }

        unsigned char sub_bits[48] = {0};
        for (int i = 0; i < 48; ++i) {
            sub_bits[i] = cd[PC2[i] - 1];
        }

        unsigned char sub_bytes[6] = {0};
        BitsToBytes(sub_bits, sub_bytes, 48);
        subkeys_.emplace_back(sub_bytes, sub_bytes + 6);
    }
}

void DesCipher::F(const unsigned char r[4], const std::vector<unsigned char>& subkey, unsigned char out[4]) {
    unsigned char e_r[6] = {0};
    Permute(r, e_r, E, 48);

    for (int i = 0; i < 6; ++i) {
        e_r[i] ^= subkey[i];
    }

    unsigned char e_bits[48] = {0};
    BytesToBits(e_r, e_bits, 48);

    unsigned char s_bits[32] = {0};
    for (int box = 0; box < 8; ++box) {
        int base = box * 6;
        int row = (e_bits[base] << 1) | e_bits[base + 5];
        int col = (e_bits[base + 1] << 3) | (e_bits[base + 2] << 2) |
                  (e_bits[base + 3] << 1) | e_bits[base + 4];
        int val = SBOX[box][row][col];

        int out_base = box * 4;
        s_bits[out_base] = static_cast<unsigned char>((val >> 3) & 0x01);
        s_bits[out_base + 1] = static_cast<unsigned char>((val >> 2) & 0x01);
        s_bits[out_base + 2] = static_cast<unsigned char>((val >> 1) & 0x01);
        s_bits[out_base + 3] = static_cast<unsigned char>(val & 0x01);
    }

    unsigned char s_bytes[4] = {0};
    BitsToBytes(s_bits, s_bytes, 32);
    Permute(s_bytes, out, P, 32);
}

void DesCipher::DesBlock(const unsigned char in[8], unsigned char out[8], bool encrypt) const {
    GenerateSubKeys();

    unsigned char ip_block[8] = {0};
    Permute(in, ip_block, IP, 64);

    unsigned char l[4] = {ip_block[0], ip_block[1], ip_block[2], ip_block[3]};
    unsigned char r[4] = {ip_block[4], ip_block[5], ip_block[6], ip_block[7]};

    for (int round = 0; round < 16; ++round) {
        unsigned char f_out[4] = {0};
        int idx = encrypt ? round : (15 - round);
        F(r, subkeys_[idx], f_out);

        unsigned char new_r[4] = {l[0], l[1], l[2], l[3]};
        Xor(new_r, f_out, 4);

        std::memcpy(l, r, 4);
        std::memcpy(r, new_r, 4);
    }

    unsigned char pre_out[8] = {r[0], r[1], r[2], r[3], l[0], l[1], l[2], l[3]};
    Permute(pre_out, out, IP_1, 64);
}

std::vector<unsigned char> DesCipher::PKCS7Pad(const std::string& input) {
    std::vector<unsigned char> data(input.begin(), input.end());
    size_t pad_len = 8 - (data.size() % 8);
    if (pad_len == 0) {
        pad_len = 8;
    }
    data.insert(data.end(), pad_len, static_cast<unsigned char>(pad_len));
    return data;
}

std::string DesCipher::PKCS7Unpad(const std::vector<unsigned char>& input) {
    if (input.empty() || input.size() % 8 != 0) {
        throw std::runtime_error("ciphertext block size error");
    }
    unsigned char pad_len = input.back();
    if (pad_len == 0 || pad_len > 8 || pad_len > input.size()) {
        throw std::runtime_error("invalid padding");
    }
    for (size_t i = 0; i < pad_len; ++i) {
        if (input[input.size() - 1 - i] != pad_len) {
            throw std::runtime_error("invalid padding");
        }
    }
    return std::string(input.begin(), input.end() - pad_len);
}

std::string DesCipher::BytesToHex(const unsigned char* in, int n) {
    std::string out;
    out.reserve(static_cast<size_t>(n) * 2);
    for (int i = 0; i < n; ++i) {
        out.push_back(ToHex((in[i] >> 4) & 0x0F));
        out.push_back(ToHex(in[i] & 0x0F));
    }
    return out;
}

std::vector<unsigned char> DesCipher::HexToBytes(const std::string& hex) {
    if (hex.size() % 2 != 0) {
        throw std::runtime_error("hex length must be even");
    }
    std::vector<unsigned char> out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        unsigned char hi = HexVal(hex[i]);
        unsigned char lo = HexVal(hex[i + 1]);
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return out;
}

std::string DesCipher::EncryptToHex(const std::string& plaintext) const {
    std::vector<unsigned char> plain = PKCS7Pad(plaintext);
    std::vector<unsigned char> cipher(plain.size());
    for (size_t i = 0; i < plain.size(); i += 8) {
        unsigned char in_block[8] = {0};
        unsigned char out_block[8] = {0};
        std::memcpy(in_block, &plain[i], 8);
        DesBlock(in_block, out_block, true);
        std::memcpy(&cipher[i], out_block, 8);
    }
    return BytesToHex(cipher.data(), static_cast<int>(cipher.size()));
}

std::string DesCipher::DecryptFromHex(const std::string& hex_ciphertext) const {
    std::vector<unsigned char> cipher = HexToBytes(hex_ciphertext);
    if (cipher.empty() || cipher.size() % 8 != 0) {
        throw std::runtime_error("ciphertext length error");
    }

    std::vector<unsigned char> plain(cipher.size());
    for (size_t i = 0; i < cipher.size(); i += 8) {
        unsigned char in_block[8] = {0};
        unsigned char out_block[8] = {0};
        std::memcpy(in_block, &cipher[i], 8);
        DesBlock(in_block, out_block, false);
        std::memcpy(&plain[i], out_block, 8);
    }

    return PKCS7Unpad(plain);
}
