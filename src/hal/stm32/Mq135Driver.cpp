// ─────────────────────────────────────────────────────────────
//  Driver du capteur MQ-135 sur STM32 — APRÈS
// ─────────────────────────────────────────────────────────────
//
// Le même code qu'à l'étape 1, à deux différences près :
//
//   1. Il tient désormais le contrat IGasSensor.
//   2. Il ne fait plus partie de la bibliothèque `core`, mais d'une
//      cible séparée que CMake ne construit QUE si on vise la carte.
//
// Rien n'a été supprimé. Le code matériel est simplement rangé du bon
// côté de la frontière : `core` ne l'inclut plus, donc `core` compile
// partout, y compris sur un serveur GitHub.
//
// Le #include ci-dessous ne résout toujours pas sur PC. C'est normal —
// et sans conséquence, puisque plus personne ne demande à ce fichier
// d'être compilé sur PC.

#include "stm32_i2c.h"

#include "core/IGasSensor.hpp"

namespace hal {

/// Lecture du MQ-135 sur le bus I²C.
class Mq135Driver final : public core::IGasSensor {
public:
    explicit Mq135Driver(std::uint8_t address) : address_(address) {}

    bool read(core::GasSample& out) override {
        std::uint16_t raw = 0;

        // L'horodatage est pris ICI, au plus près de la conversion —
        // avant que l'ordonnanceur n'ait le temps d'introduire du retard.
        const std::uint64_t t = micros_since_boot();

        if (!i2c_read_register(address_, kRawValueRegister, &raw, sizeof(raw))) {
            return false;                     // échec de communication
        }

        const float ppm = static_cast<float>(raw) * kCountsToPpm;

        out.concentration_ppm = ppm;
        out.timestamp_us      = t;
        out.health            = (ppm > kMaxValidPpm)
                                    ? core::SensorHealth::OutOfRange
                                    : core::SensorHealth::Ok;
        return true;
    }

private:
    static constexpr std::uint8_t kRawValueRegister = 0x00;
    static constexpr float        kCountsToPpm      = 0.4882f;
    static constexpr float        kMaxValidPpm      = 10000.0f;

    std::uint8_t address_;
};

}  // namespace hal
