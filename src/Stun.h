//
// Created by xiang on 2020/12/2.
//




#ifndef WEBRTC_SERVER_STUN_H
#define WEBRTC_SERVER_STUN_H

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Stun
{
public:
    // STUN message class.
    enum class Class : uint16_t
    {
        REQUEST          = 0,
        INDICATION       = 1,
        SUCCESS_RESPONSE = 2,
        ERROR_RESPONSE   = 3
    };

    // STUN message method.
    enum class Method : uint16_t
    {
        BINDING = 1
    };

    // Attribute type.
    enum class Attribute : uint16_t
    {
        MAPPED_ADDRESS     = 0x0001,
        USERNAME           = 0x0006,
        MESSAGE_INTEGRITY  = 0x0008,
        ERROR_CODE         = 0x0009,
        UNKNOWN_ATTRIBUTES = 0x000A,
        REALM              = 0x0014,
        NONCE              = 0x0015,
        XOR_MAPPED_ADDRESS = 0x0020,
        PRIORITY           = 0x0024,
        USE_CANDIDATE      = 0x0025,
        SOFTWARE           = 0x8022,
        ALTERNATE_SERVER   = 0x8023,
        FINGERPRINT        = 0x8028,
        ICE_CONTROLLED     = 0x8029,
        ICE_CONTROLLING    = 0x802A
    };

    // Authentication result.
    enum class Authentication
    {
        OK           = 0,
        UNAUTHORIZED = 1,
        BAD_REQUEST  = 2
    };

public:
    static bool IsStun(const uint8_t* data, size_t len);
    static Stun* Parse(const uint8_t* data, size_t len);

private:
    static const uint8_t magicCookie[];

public:
    Stun(
            Class klass, Method method, const uint8_t* transactionId, const uint8_t* data, size_t size);
    ~Stun();

    void Dump() const;
    Class GetClass() const;
    Method GetMethod() const;
    const uint8_t* GetData() const;
    size_t GetSize() const;
    void SetUsername(const char* username, size_t len);
    void SetPriority(uint32_t priority);
    void SetIceControlling(uint64_t iceControlling);
    void SetIceControlled(uint64_t iceControlled);
    void SetUseCandidate();
    void SetXorMappedAddress(const struct sockaddr* xorMappedAddress);
    void SetErrorCode(uint16_t errorCode);
    void SetMessageIntegrity(const uint8_t* messageIntegrity);
    void SetFingerprint();
    const std::string& GetUsername() const;
    uint32_t GetPriority() const;
    uint64_t GetIceControlling() const;
    uint64_t GetIceControlled() const;
    bool HasUseCandidate() const;
    uint16_t GetErrorCode() const;
    bool HasMessageIntegrity() const;
    bool HasFingerprint() const;
    Authentication CheckAuthentication(
            const std::string& localUsername, const std::string& localPassword);
    Stun* CreateSuccessResponse();
    Stun* CreateErrorResponse(uint16_t errorCode);
    void Authenticate(const std::string& password);
    void Serialize(uint8_t* buffer);

private:
    // Passed by argument.
    Class klass;                             // 2 bytes.
    Method method;                           // 2 bytes.
    const uint8_t* transactionId{ nullptr }; // 12 bytes.
    uint8_t* data{ nullptr };                // Pointer to binary data.
    size_t size{ 0 };                        // The full message size (including header).
    // STUN attributes.
    std::string username;                               // Less than 513 bytes.
    uint32_t priority{ 0 };                             // 4 bytes unsigned integer.
    uint64_t iceControlling{ 0 };                       // 8 bytes unsigned integer.
    uint64_t iceControlled{ 0 };                        // 8 bytes unsigned integer.
    bool hasUseCandidate{ false };                      // 0 bytes.
    const uint8_t* messageIntegrity{ nullptr };         // 20 bytes.
    bool hasFingerprint{ false };                       // 4 bytes.
    const struct sockaddr* xorMappedAddress{ nullptr }; // 8 or 20 bytes.
    uint16_t errorCode{ 0 };                            // 4 bytes (no reason phrase).
    std::string password;
};

/* Inline class methods. */

inline bool Stun::IsStun(const uint8_t* data, size_t len)
{
    // clang-format off
    return (
            // STUN headers are 20 bytes.
            (len >= 20) &&
            // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
            (data[0] < 3) &&
            // Magic cookie must match.
            (data[4] == Stun::magicCookie[0]) && (data[5] == Stun::magicCookie[1]) &&
            (data[6] == Stun::magicCookie[2]) && (data[7] == Stun::magicCookie[3])
    );
    // clang-format on
}

/* Inline instance methods. */

inline Stun::Class Stun::GetClass() const
{
    return this->klass;
}

inline Stun::Method Stun::GetMethod() const
{
    return this->method;
}

inline const uint8_t* Stun::GetData() const
{
    return this->data;
}

inline size_t Stun::GetSize() const
{
    return this->size;
}

inline void Stun::SetUsername(const char* username, size_t len)
{
    this->username.assign(username, len);
}

inline void Stun::SetPriority(const uint32_t priority)
{
    this->priority = priority;
}

inline void Stun::SetIceControlling(const uint64_t iceControlling)
{
    this->iceControlling = iceControlling;
}

inline void Stun::SetIceControlled(const uint64_t iceControlled)
{
    this->iceControlled = iceControlled;
}

inline void Stun::SetUseCandidate()
{
    this->hasUseCandidate = true;
}

inline void Stun::SetXorMappedAddress(const struct sockaddr* xorMappedAddress)
{
    this->xorMappedAddress = xorMappedAddress;
}

inline void Stun::SetErrorCode(uint16_t errorCode)
{
    this->errorCode = errorCode;
}

inline void Stun::SetMessageIntegrity(const uint8_t* messageIntegrity)
{
    this->messageIntegrity = messageIntegrity;
}

inline void Stun::SetFingerprint()
{
    this->hasFingerprint = true;
}

inline const std::string& Stun::GetUsername() const
{
    return this->username;
}

inline uint32_t Stun::GetPriority() const
{
    return this->priority;
}

inline uint64_t Stun::GetIceControlling() const
{
    return this->iceControlling;
}

inline uint64_t Stun::GetIceControlled() const
{
    return this->iceControlled;
}

inline bool Stun::HasUseCandidate() const
{
    return this->hasUseCandidate;
}

inline uint16_t Stun::GetErrorCode() const
{
    return this->errorCode;
}

inline bool Stun::HasMessageIntegrity() const
{
    return (this->messageIntegrity ? true : false);
}

inline bool Stun::HasFingerprint() const
{
    return this->hasFingerprint;
}


#endif //WEBRTC_SERVER_STUN_H
