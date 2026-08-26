#include "core/MavlinkEncoder.hpp"

#include <cstring>

namespace core {
namespace {

// Numéros de message et empreintes de définition, tirés du dialecte
// « common » de MAVLink. Ces valeurs ne s'inventent pas : elles sont
// générées à partir des fichiers XML du protocole.
constexpr std::uint32_t kMsgHeartbeat        = 0;
constexpr std::uint8_t  kCrcHeartbeat        = 50;

constexpr std::uint32_t kMsgNamedValueFloat  = 251;
constexpr std::uint8_t  kCrcNamedValueFloat  = 170;

constexpr std::uint32_t kMsgStatustext       = 253;
constexpr std::uint8_t  kCrcStatustext       = 83;

constexpr std::uint8_t  kStx                 = 0xFD;
constexpr std::size_t   kHeaderLen           = 10;
constexpr std::size_t   kChecksumLen         = 2;

/// CRC-16/X.25, l'algorithme imposé par MAVLink.
///
/// Une somme de contrôle sert à détecter la corruption : sur une liaison
/// radio, un octet sur des milliers arrive faux. Sans vérification, une
/// concentration de 400 ppm pourrait être reçue comme 400 000.
void crc_accumulate(std::uint8_t data, std::uint16_t* crc) {
    std::uint8_t tmp = data ^ static_cast<std::uint8_t>(*crc & 0xFF);
    tmp ^= static_cast<std::uint8_t>(tmp << 4);
    *crc = static_cast<std::uint16_t>((*crc >> 8) ^ (tmp << 8) ^ (tmp << 3) ^ (tmp >> 4));
}

}  // namespace

MavlinkEncoder::MavlinkEncoder(std::uint8_t system_id, std::uint8_t component_id)
    : system_id_(system_id), component_id_(component_id), seq_(0) {}

std::size_t MavlinkEncoder::frame(std::uint32_t msgid, std::uint8_t crc_extra,
                                  const std::uint8_t* payload, std::size_t payload_len,
                                  std::uint8_t* out, std::size_t capacity) {
    // Troncature : on retire les octets nuls de fin, en gardant au moins un
    // octet de charge utile.
    while (payload_len > 1 && payload[payload_len - 1] == 0) --payload_len;

    const std::size_t total = kHeaderLen + payload_len + kChecksumLen;
    if (capacity < total) return 0;

    out[0] = kStx;
    out[1] = static_cast<std::uint8_t>(payload_len);
    out[2] = 0;                                  // drapeaux d'incompatibilité
    out[3] = 0;                                  // drapeaux de compatibilité
    out[4] = seq_;
    out[5] = system_id_;
    out[6] = component_id_;
    out[7] = static_cast<std::uint8_t>(msgid & 0xFF);
    out[8] = static_cast<std::uint8_t>((msgid >> 8) & 0xFF);
    out[9] = static_cast<std::uint8_t>((msgid >> 16) & 0xFF);
    std::memcpy(out + kHeaderLen, payload, payload_len);

    // Le CRC couvre tout SAUF le marqueur de début, plus le CRC_EXTRA.
    std::uint16_t crc = 0xFFFF;
    for (std::size_t i = 1; i < kHeaderLen + payload_len; ++i) {
        crc_accumulate(out[i], &crc);
    }
    crc_accumulate(crc_extra, &crc);

    out[kHeaderLen + payload_len]     = static_cast<std::uint8_t>(crc & 0xFF);
    out[kHeaderLen + payload_len + 1] = static_cast<std::uint8_t>(crc >> 8);

    ++seq_;                                      // débordement voulu à 255→0
    return total;
}

std::size_t MavlinkEncoder::encode_named_value_float(std::uint32_t time_boot_ms,
                                                     const char* name, float value,
                                                     std::uint8_t* out,
                                                     std::size_t capacity) {
    // Ordre sur le fil : uint32, float, char[10] — par taille décroissante,
    // et NON dans l'ordre de la documentation.
    std::uint8_t payload[18] = {};
    std::memcpy(payload + 0, &time_boot_ms, 4);
    std::memcpy(payload + 4, &value, 4);
    std::strncpy(reinterpret_cast<char*>(payload + 8), name, 10);

    return frame(kMsgNamedValueFloat, kCrcNamedValueFloat, payload, sizeof(payload),
                 out, capacity);
}

std::size_t MavlinkEncoder::encode_statustext(std::uint8_t severity, const char* text,
                                              std::uint8_t* out, std::size_t capacity) {
    // 1 octet de sévérité + 50 de texte + 2 d'extensions v2.
    std::uint8_t payload[53] = {};
    payload[0] = severity;
    std::strncpy(reinterpret_cast<char*>(payload + 1), text, 50);

    return frame(kMsgStatustext, kCrcStatustext, payload, sizeof(payload), out, capacity);
}

std::size_t MavlinkEncoder::encode_heartbeat(std::uint8_t* out, std::size_t capacity,
                                             std::uint8_t type, std::uint8_t autopilot) {
    std::uint8_t payload[9] = {};
    // custom_mode (uint32) en tête : toujours le champ le plus large d'abord.
    payload[4] = type;        // 7 = MAV_TYPE_AIRSHIP
    payload[5] = autopilot;   // 0 = MAV_AUTOPILOT_GENERIC
    payload[6] = 0;           // base_mode
    payload[7] = 4;           // system_status = MAV_STATE_ACTIVE
    payload[8] = 3;           // version du protocole MAVLink

    return frame(kMsgHeartbeat, kCrcHeartbeat, payload, sizeof(payload), out, capacity);
}

}  // namespace core
