#pragma once

#include "core/GasSample.hpp"

namespace core {

/// Contrat entre la logique de vol et n'importe quel capteur de gaz.
///
/// ─────────────────────────────────────────────────────────────
///  C'EST LE FICHIER LE PLUS IMPORTANT DU PROJET
/// ─────────────────────────────────────────────────────────────
///
///
/// Ce fichier est une abstraction, en miniature.
///
/// LE PRINCIPE : on abstrait ce qui change, on expose ce qui est stable.
///
///   Ce qui change  → le modèle du capteur, le bus, les registres,
///                    le fabricant, la carte
///   Ce qui est stable → le concept « une concentration de gaz, en ppm,
///                    horodatée, avec un indice de fiabilité »
///
/// L'interface se définit donc dans le vocabulaire du DOMAINE, jamais
/// dans celui du matériel. Compare :
///
///     uint16_t lire_registre(uint8_t adresse);   // ✗ l'appelant doit
///                                                //   connaître le matériel
///     bool read(GasSample& out);                 // ✓ l'appelant connaît
///                                                //   le problème
///
/// Une classe qui n'a que des méthodes `= 0` et un destructeur s'appelle
/// une INTERFACE : une promesse sans réalisation. Deux classes la
/// tiendront — le vrai driver sur la carte, un faux capteur dans les
/// tests. Le code qui s'en sert ne fait pas la différence, et c'est
/// précisément ce qui le rend testable sans matériel.
class IGasSensor {
public:
    /// Destructeur virtuel : indispensable dès qu'on hérite.
    /// Sans lui, détruire l'objet à travers un pointeur d'interface
    /// n'appellerait pas le destructeur de la classe fille — une fuite
    /// de ressources silencieuse, et un classique des entretiens C++.
    virtual ~IGasSensor() = default;

    /// Tente une mesure.
    /// @param out  rempli seulement si la lecture aboutit
    /// @return     false si la communication elle-même a échoué
    ///
    /// Deux niveaux d'échec, volontairement distincts :
    ///   • `false`                     → on n'a pas pu parler au capteur
    ///   • `true` + health != Ok       → il a répondu, mais mal
    /// Ce n'est pas la même panne, et le diagnostic au sol en dépend.
    virtual bool read(GasSample& out) = 0;
};

}  // namespace core
