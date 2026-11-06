#ifndef SHA256_H
#define SHA256_H

#include <string>
#include <vector>
#include <cstdint>

namespace security {

class SHA256 {
private:
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];

    void transform(const uint8_t data[64]);

public:
    SHA256();
    void reset();
    void update(const uint8_t* data, size_t len);
    void update(const std::string& str);
    std::vector<uint8_t> digest();
    std::string digestRaw();
    std::string digestHex();
};

// Convenience helper functions
std::string sha256Hex(const std::string& input);
std::string sha256Raw(const std::string& input);

// RFC 2104 HMAC-SHA256
std::string hmacSha256Raw(const std::string& key, const std::string& data);
std::string hmacSha256Hex(const std::string& key, const std::string& data);

// Base64URL (RFC 4648 §5)
std::string base64UrlEncode(const std::string& data);
std::string base64UrlDecode(const std::string& data);
bool isValidBase64Url(const std::string& input);

} // namespace security

#endif // SHA256_H
