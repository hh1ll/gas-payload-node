#pragma once

#include "core/ITelemetrySink.hpp"
#include "core/MavlinkEncoder.hpp"
#include "core/PayloadService.hpp"

namespace core {

/// Décide QUOI émettre, et QUAND.
///
/// ─────────────────────────────────────────────────────────────
///  DEUX RÉGIMES DE DONNÉES
/// ─────────────────────────────────────────────────────────────
///
/// Une liaison radio de drone tourne souvent à 57 600 bauds — quelques
/// kilo-octets par seconde, partagés entre la télémétrie de vol, les
/// commandes, le GPS et ta charge utile. C'est peu, et ça se remplit vite.
///
/// D'où deux régimes distincts, à ne jamais confondre :
///
///   TÉLÉMÉTRIE (continu)  → NAMED_VALUE_FLOAT, à chaque cycle.
///                           « voici la valeur actuelle »
///
///   ÉVÉNEMENT (rare)      → STATUSTEXT, UNIQUEMENT quand quelque chose
///                           change. « il vient de se passer ceci »
///
/// Envoyer un STATUSTEXT à chaque cycle saturerait la liaison et noierait
/// l'opérateur sous des messages identiques — au point qu'il cesserait de
/// les lire.
///
/// Tu reconnais le raisonnement : c'est exactement celui de l'hystérésis
/// en séance 1. Une alarme qui crie tout le temps ne dit plus rien. Là on
/// protège l'attention d'un humain ET la bande passante d'une radio.
class TelemetryReporter {
public:
    TelemetryReporter(ITelemetrySink& sink, MavlinkEncoder& encoder);

    /// Un cycle d'émission.
    /// @param service       l'état courant de la charge utile
    /// @param time_boot_ms  millisecondes depuis le démarrage
    void report(const PayloadService& service, std::uint32_t time_boot_ms);

    /// Signe de vie, à envoyer environ une fois par seconde.
    /// Sans lui, QGroundControl n'affichera jamais le véhicule.
    void send_heartbeat();

private:
    ITelemetrySink& sink_;
    MavlinkEncoder& encoder_;

    /// Ce qu'on a émis la dernière fois — sert à détecter les CHANGEMENTS.
    AlertState last_reported_alert_;
    bool       first_report_;
};

}  // namespace core
