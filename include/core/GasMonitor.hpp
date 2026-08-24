#pragma once

namespace core {

/// État d'alerte de la charge utile.
enum class AlertState {
    Normal,   ///< concentration considérée comme sûre
    Alert     ///< fuite probable, à signaler au sol
};

/// Surveille une concentration de gaz et décide s'il faut lever une alerte.
///
/// LE PROBLÈME QU'ON RÉSOUT ICI
/// ────────────────────────────
/// Avec un seuil unique, un capteur qui oscille autour de ce seuil fait
/// basculer l'alerte des dizaines de fois par seconde. Au sol, l'opérateur
/// voit l'alarme clignoter et finit par l'ignorer — le pire résultat possible
/// pour un système de sécurité.
///
/// La solution classique est l'HYSTÉRÉSIS : deux seuils au lieu d'un.
/// On déclenche haut, on relâche bas. Entre les deux, on ne change rien.
///
///     concentration
///          ▲
///  rising  ┤ ─ ─ ─ ─ ─ ─┬─────────────  au-dessus : Alert
///          │            │
///          │   zone     │   ← dans la bande : on GARDE l'état courant
///          │  morte     │
///  falling ┤ ─ ─ ─ ─ ─ ─┴─────────────  en dessous : Normal
///          │
///          └──────────────────────────▶ temps
///
/// Cette classe ne connaît ni capteur, ni bus, ni carte. Elle prend des
/// nombres et renvoie une décision. C'est précisément ce qui la rend
/// testable sur ton PC — et, en séance 2, sur un serveur GitHub.
class GasMonitor {
public:
    /// @param rising_ppm   seuil de déclenchement (concentration haute)
    /// @param falling_ppm  seuil de retour au calme (concentration basse)
    ///
    /// On attend rising_ppm > falling_ppm. Ce que fait la classe si ce
    /// n'est pas le cas est une décision de conception : c'est à toi de
    /// la prendre, et de la documenter par un test.
    GasMonitor(float rising_ppm, float falling_ppm);

    /// Intègre un nouvel échantillon et renvoie l'état résultant.
    AlertState update(float concentration_ppm);

    /// Renvoie l'état courant sans consommer d'échantillon.
    AlertState state() const;

private:
    float      rising_ppm_;
    float      falling_ppm_;
    AlertState state_;
};

}  // namespace core
