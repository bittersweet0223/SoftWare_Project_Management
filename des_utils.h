#ifndef DES_UTILS_H
#define DES_UTILS_H

#include <string>
#include <vector>

class DesCipher {
public:
    // key_text: 任意长度字符串，自动截断/补零为8字节
    explicit DesCipher(const std::string& key_text);

    // 明文加密为HEX字符串
    std::string EncryptToHex(const std::string& plaintext) const;
    // HEX密文解密为明文
    std::string DecryptFromHex(const std::string& hex_ciphertext) const;

private:
    // 8字节主密钥
    unsigned char key_[8]{};
    // 16轮子密钥（每轮48bit=6字节）
    mutable std::vector<std::vector<unsigned char>> subkeys_;

    void GenerateSubKeys() const;
    void DesBlock(const unsigned char in[8], unsigned char out[8], bool encrypt) const;
    static void Permute(const unsigned char* in, unsigned char* out, const int* table, int n);
    static void Xor(unsigned char* a, const unsigned char* b, int n);
    static void F(const unsigned char r[4], const std::vector<unsigned char>& subkey, unsigned char out[4]);
    static void BytesToBits(const unsigned char* in, unsigned char* out, int n);
    static void BitsToBytes(const unsigned char* in, unsigned char* out, int n);
    static std::string BytesToHex(const unsigned char* in, int n);
    static std::vector<unsigned char> HexToBytes(const std::string& hex);
    static std::vector<unsigned char> PKCS7Pad(const std::string& input);
    static std::string PKCS7Unpad(const std::vector<unsigned char>& input);
};

#endif
