#pragma once

#include "core/GasMonitor.hpp"
#include "core/GasSample.hpp"
#include "core/IGasSensor.hpp"

namespace core {

/// Santé du service, telle qu'on la rapporterait au sol.
///
/// Trois niveaux plutôt que deux : entre « tout va bien » et « le capteur
/// est mort », il y a « ça hoquette ». Un opérateur au sol n'agit pas
/// pareil selon les deux — d'où la dégradation graduée.
enum class ServiceHealth {
    Ok,        ///< dernière lecture réussie
    Degraded,  ///< quelques échecs consécutifs, sous le seuil
    Lost       ///< trop d'échecs : on ne fait plus confiance au capteur
};

/// Orchestre le capteur et la décision d'alerte.
///
/// ─────────────────────────────────────────────────────────────
///  L'INJECTION DE DÉPENDANCES
/// ─────────────────────────────────────────────────────────────
///
/// Regarde le constructeur : il REÇOIT un capteur. Il ne le fabrique
/// pas, il ne va pas le chercher, il ne sait même pas lequel c'est.
///
/// Cette inversion porte un nom — l'injection de dépendances — et c'est
/// la technique qui rend tout le reste testable. Compare :
///
///     // ✗ l'objet va chercher ce dont il a besoin
///     PayloadService() { capteur_ = new Mq135Driver(); }
///     // → intestable : sans carte, ça ne marche pas
///
///     // ✓ on lui donne
///     PayloadService(IGasSensor& sensor);
///     // → en production on passe le vrai driver,
///     //   dans les tests on passe un faux
///
/// C'est aussi ce qui permet de changer de capteur sans toucher une
/// ligne de cette classe.
class PayloadService {
public:
    /// @param sensor               la source de mesures (référence : le
    ///                             service ne la possède pas, il l'utilise)
    /// @param monitor              la logique de décision, déjà réglée
    /// @param failures_before_lost nombre d'échecs consécutifs avant de
    ///                             déclarer le capteur perdu
    PayloadService(IGasSensor& sensor, GasMonitor monitor,
                   int failures_before_lost);

    /// Une itération : lit le capteur, met à jour l'état.
    /// Appelée en boucle par le programme principal.
    void poll();

    AlertState     alert()  const;
    ServiceHealth  health() const;

    /// La dernière mesure jugée exploitable. Reste figée pendant les pannes.
    const GasSample& last_valid_sample() const;

    int consecutive_failures() const;

private:
    IGasSensor&   sensor_;
    GasMonitor    monitor_;
    int           failures_before_lost_;
    int           consecutive_failures_;
    ServiceHealth health_;
    GasSample     last_valid_;
};

}  // namespace core
