#pragma once

#include <cstdint>
#include <vector>

#include "core/ITelemetrySink.hpp"

/// Une radio qui n'émet nulle part : elle garde tout en mémoire.
///
/// Même idée que FakeGasSensor, à l'autre bout de la chaîne. Grâce à elle
/// tu peux vérifier EXACTEMENT ce que ton code a émis — combien de
/// trames, de quel type, avec quel contenu — sans radio, sans drone et
/// sans station sol.
class FakeTelemetrySink : public core::ITelemetrySink {
public:
    bool send(const std::uint8_t* data, std::size_t length) override {
        if (fail_next_) { fail_next_ = false; return false; }
        frames_.emplace_back(data, data + length);
        return true;
    }

    /// Fait échouer la prochaine émission — pour tester la liaison coupée.
    void fail_next_send() { fail_next_ = true; }

    std::size_t frame_count() const { return frames_.size(); }

    const std::vector<std::uint8_t>& frame(std::size_t i) const { return frames_[i]; }

    /// Le numéro de message MAVLink d'une trame (octets 7 à 9).
    std::uint32_t message_id(std::size_t i) const {
        const auto& f = frames_[i];
        if (f.size() < 10) return 0xFFFFFFFF;
        return static_cast<std::uint32_t>(f[7])
             | (static_cast<std::uint32_t>(f[8]) << 8)
             | (static_cast<std::uint32_t>(f[9]) << 16);
    }

    /// Combien de trames de ce type ont été émises ?
    std::size_t count_of(std::uint32_t msgid) const {
        std::size_t n = 0;
        for (std::size_t i = 0; i < frames_.size(); ++i) {
            if (message_id(i) == msgid) ++n;
        }
        return n;
    }

    void clear() { frames_.clear(); }

    static constexpr std::uint32_t kHeartbeat       = 0;
    static constexpr std::uint32_t kNamedValueFloat = 251;
    static constexpr std::uint32_t kStatustext      = 253;

private:
    std::vector<std::vector<std::uint8_t>> frames_;
    bool fail_next_ = false;
};
