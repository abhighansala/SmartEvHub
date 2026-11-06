#include "sha256.h"
#include <cstring>
#include <iomanip>
#include <sstream>

namespace security {

static inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static inline uint32_t choose(uint32_t e, uint32_t f, uint32_t g) {
    return (e & f) ^ (~e & g);
}

static inline uint32_t majority(uint32_t a, uint32_t b, uint32_t c) {
    return (a & b) ^ (a & c) ^ (b & c);
}

static inline uint32_t sig0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static inline uint32_t sig1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static inline uint32_t theta0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static inline uint32_t theta1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

SHA256::SHA256() {
    reset();
}

void SHA256::reset() {
    state[0] = 0x6a09e667;
    state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372;
    state[3] = 0xa54ff53a;
    state[4] = 0x510e527f;
    state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab;
    state[7] = 0x5be0cd19;
    count = 0;
    std::memset(buffer, 0, sizeof(buffer));
}

void SHA256::transform(const uint8_t data[64]) {
    uint32_t m[64];
    for (int i = 0; i < 16; ++i) {
        m[i] = (static_cast<uint32_t>(data[i * 4]) << 24) |
               (static_cast<uint32_t>(data[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(data[i * 4 + 2]) << 8) |
               (static_cast<uint32_t>(data[i * 4 + 3]));
    }
    for (int i = 16; i < 64; ++i) {
        m[i] = theta1(m[i - 2]) + m[i - 7] + theta0(m[i - 15]) + m[i - 16];
    }

    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + sig1(e) + choose(e, f, g) + K[i] + m[i];
        uint32_t t2 = sig0(a) + majority(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void SHA256::update(const uint8_t* data, size_t len) {
    size_t bufferIndex = static_cast<size_t>(count % 64);
    count += len;

    size_t partLen = 64 - bufferIndex;
    size_t i = 0;

    if (len >= partLen) {
        std::memcpy(&buffer[bufferIndex], data, partLen);
        transform(buffer);
        for (i = partLen; i + 63 < len; i += 64) {
            transform(&data[i]);
        }
        bufferIndex = 0;
    }
    if (i < len) {
        std::memcpy(&buffer[bufferIndex], &data[i], len - i);
    }
}

void SHA256::update(const std::string& str) {
    update(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

std::vector<uint8_t> SHA256::digest() {
    uint8_t padBuffer[128];
    size_t bufferIndex = static_cast<size_t>(count % 64);
    size_t padLen = (bufferIndex < 56) ? (56 - bufferIndex) : (120 - bufferIndex);

    std::memset(padBuffer, 0, sizeof(padBuffer));
    padBuffer[0] = 0x80;

    uint64_t bits = count * 8;
    uint8_t bitsBytes[8];
    for (int i = 0; i < 8; ++i) {
        bitsBytes[7 - i] = static_cast<uint8_t>(bits >> (i * 8));
    }

    update(padBuffer, padLen);
    update(bitsBytes, 8);

    std::vector<uint8_t> out(32);
    for (int i = 0; i < 8; ++i) {
        out[i * 4]     = static_cast<uint8_t>(state[i] >> 24);
        out[i * 4 + 1] = static_cast<uint8_t>(state[i] >> 16);
        out[i * 4 + 2] = static_cast<uint8_t>(state[i] >> 8);
        out[i * 4 + 3] = static_cast<uint8_t>(state[i]);
    }
    reset();
    return out;
}

std::string SHA256::digestRaw() {
    std::vector<uint8_t> d = digest();
    return std::string(reinterpret_cast<const char*>(d.data()), d.size());
}

std::string SHA256::digestHex() {
    std::vector<uint8_t> d = digest();
    std::ostringstream ss;
    for (uint8_t b : d) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return ss.str();
}

std::string sha256Hex(const std::string& input) {
    SHA256 sha;
    sha.update(input);
    return sha.digestHex();
}

std::string sha256Raw(const std::string& input) {
    SHA256 sha;
    sha.update(input);
    return sha.digestRaw();
}

std::string hmacSha256Raw(const std::string& key, const std::string& data) {
    std::string k = key;
    if (k.size() > 64) {
        k = sha256Raw(k);
    }
    if (k.size() < 64) {
        k.resize(64, '\0');
    }

    std::string k_ipad(64, '\0');
    std::string k_opad(64, '\0');
    for (size_t i = 0; i < 64; ++i) {
        k_ipad[i] = static_cast<char>(k[i] ^ 0x36);
        k_opad[i] = static_cast<char>(k[i] ^ 0x5c);
    }

    SHA256 innerSha;
    innerSha.update(k_ipad);
    innerSha.update(data);
    std::string innerDigest = innerSha.digestRaw();

    SHA256 outerSha;
    outerSha.update(k_opad);
    outerSha.update(innerDigest);
    return outerSha.digestRaw();
}

std::string hmacSha256Hex(const std::string& key, const std::string& data) {
    std::string raw = hmacSha256Raw(key, data);
    std::ostringstream ss;
    for (unsigned char b : raw) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(b);
    }
    return ss.str();
}

// Base64URL
static const char BASE64URL_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

bool isValidBase64Url(const std::string& input) {
    if (input.empty()) return false;
    for (char c : input) {
        if (!((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '-' || c == '_' || c == '=')) {
            return false;
        }
    }
    return true;
}

std::string base64UrlEncode(const std::string& data) {
    std::string out;
    size_t len = data.size();
    out.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3) {
        uint32_t b0 = static_cast<uint8_t>(data[i]);
        uint32_t b1 = (i + 1 < len) ? static_cast<uint8_t>(data[i + 1]) : 0;
        uint32_t b2 = (i + 2 < len) ? static_cast<uint8_t>(data[i + 2]) : 0;

        uint32_t triple = (b0 << 16) | (b1 << 8) | b2;

        out.push_back(BASE64URL_CHARS[(triple >> 18) & 0x3F]);
        out.push_back(BASE64URL_CHARS[(triple >> 12) & 0x3F]);
        if (i + 1 < len) {
            out.push_back(BASE64URL_CHARS[(triple >> 6) & 0x3F]);
        }
        if (i + 2 < len) {
            out.push_back(BASE64URL_CHARS[triple & 0x3F]);
        }
    }
    return out;
}

std::string base64UrlDecode(const std::string& input) {
    if (!isValidBase64Url(input)) {
        return "";
    }
    auto decodeChar = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '-' || c == '+') return 62;
        if (c == '_' || c == '/') return 63;
        return -1;
    };

    std::string out;
    uint32_t val = 0;
    int valb = -8;
    for (char c : input) {
        if (c == '=') break;
        int d = decodeChar(c);
        if (d == -1) return "";
        val = (val << 6) | d;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

} // namespace security
