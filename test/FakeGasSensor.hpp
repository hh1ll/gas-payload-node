#pragma once

#include <cstddef>
#include <vector>

#include "core/IGasSensor.hpp"

/// Un capteur de gaz qui n'existe pas.
///
/// Il tient la même promesse que le vrai — il hérite de IGasSensor — mais
/// au lieu de parler à un bus I²C, il rejoue un scénario que TU écris.
///
/// C'est ce qu'on appelle un FAKE : une implémentation d'interface écrite
/// pour les tests. Sa valeur est qu'il te donne un pouvoir que le vrai
/// capteur ne te donnera jamais — celui de provoquer une panne à la
/// demande, exactement au moment que tu choisis.
///
/// Comment produirais-tu une fuite de méthane suivie d'une panne capteur,
/// à la trentième mesure, dans une salle de TP ? Tu ne le ferais pas.
/// Ici, c'est trois lignes.
class FakeGasSensor : public core::IGasSensor {
public:
    /// Programme une mesure réussie.
    void push(float ppm, core::SensorHealth health = core::SensorHealth::Ok) {
        script_.push_back({ppm, health, true});
    }

    /// Programme une panne de communication : read() renverra false.
    void push_read_failure() {
        script_.push_back({0.0f, core::SensorHealth::Fault, false});
    }

    /// Programme n mesures identiques d'affilée. Confort d'écriture.
    void push_many(int count, float ppm,
                   core::SensorHealth health = core::SensorHealth::Ok) {
        for (int i = 0; i < count; ++i) push(ppm, health);
    }

    bool read(core::GasSample& out) override {
        ++read_count_;

        // Scénario épuisé : on rejoue la dernière entrée indéfiniment,
        // ce qui évite d'avoir à compter les appels au ppm près.
        if (script_.empty()) return false;

        const std::size_t i =
            (cursor_ < script_.size()) ? cursor_ : script_.size() - 1;
        if (cursor_ < script_.size()) ++cursor_;

        const Entry& e = script_[i];
        if (!e.succeeds) return false;

        out.concentration_ppm = e.ppm;
        out.health            = e.health;
        out.timestamp_us      = 1000ull * read_count_;   // horloge factice
        return true;
    }

    /// Combien de fois le service a-t-il interrogé le capteur ?
    int read_count() const { return read_count_; }

private:
    struct Entry {
        float              ppm;
        core::SensorHealth health;
        bool               succeeds;
    };

    std::vector<Entry> script_;
    std::size_t        cursor_     = 0;
    int                read_count_ = 0;
};
