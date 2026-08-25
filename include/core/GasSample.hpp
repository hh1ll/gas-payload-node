#pragma once

#include <cstdint>

namespace core {

/// État de santé rapporté par le capteur lui-même.
///
/// Point important : une panne capteur n'est PAS une exception, c'est un
/// mode de fonctionnement normal en vol. Un capteur chauffe, se salit,
/// perd son alimentation une milliseconde, sort de sa plage. Le code qui
/// l'utilise doit savoir le gérer sans s'arrêter — donc la santé fait
/// partie de la donnée, elle n'est pas signalée à côté.
enum class SensorHealth {
    Ok,           ///< mesure exploitable
    OutOfRange,   ///< le capteur répond, mais hors de sa plage de validité
    Fault         ///< le capteur est en défaut, la valeur n'a aucun sens
};

/// Une mesure de concentration de gaz.
///
/// Trois détails de conception à savoir.
///
/// 1. L'UNITÉ EST DANS LE NOM DU CHAMP (`_ppm`, `_us`).
///    Pratique standard en aérospatiale. Les confusions d'unités ont
///    détruit des engins réels ; un nom de champ coûte moins cher.
///
/// 2. L'HORODATAGE VIENT DE LA CAPTURE, PAS DE LA LECTURE.
///    C'est le driver qui le pose, au plus près du matériel. Justification
///    matérielle : entre le moment où le capteur convertit et celui où ta
///    boucle lit la valeur, il y a le délai du bus et l'ordonnancement des
///    tâches — une gigue variable, qui fausse toute fusion de données.
///
/// 3. LA SANTÉ FAIT PARTIE DE LA STRUCTURE.
///    Impossible de lire une mesure sans voir si elle est fiable.
struct GasSample {
    float        concentration_ppm = 0.0f;
    std::uint64_t timestamp_us     = 0;
    SensorHealth health            = SensorHealth::Fault;
};

}  // namespace core
