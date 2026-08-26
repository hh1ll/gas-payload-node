#pragma once

#include <cstddef>
#include <cstdint>

namespace core {

/// Fabrique des trames MAVLink v2.
///
/// ─────────────────────────────────────────────────────────────
///  ANATOMIE D'UNE TRAME MAVLINK v2
/// ─────────────────────────────────────────────────────────────
///
///   ┌────┬─────┬────┬────┬─────┬─────┬──────┬─────────┬─────────┬─────┐
///   │ FD │ len │ if │ cf │ seq │ sys │ comp │ msgid×3 │ payload │ crc │
///   └────┴─────┴────┴────┴─────┴─────┴──────┴─────────┴─────────┴─────┘
///     0     1     2    3     4     5     6      7..9     10..      ×2
///
///   FD      marqueur de début (0xFD pour la v2 ; 0xFE en v1)
///   len     longueur de la charge utile
///   if/cf   drapeaux de compatibilité, à zéro dans notre cas
///   seq     compteur de séquence — c'est lui qui permet au sol de
///           détecter les messages PERDUS. Sur une liaison radio, il y en
///           a toujours.
///   sys     identifiant du véhicule (1 = notre aéronef)
///   comp    identifiant du composant à bord (25 = notre charge utile)
///   msgid   numéro du message, sur 3 octets en petit-boutiste
///   crc     somme de contrôle sur 2 octets
///
/// TROIS SUBTILITÉS QUI FONT ÉCHOUER 90 % DES IMPLÉMENTATIONS MAISON —
/// et il n'y a aucun message d'erreur quand on se trompe : la station sol
/// jette simplement la trame en silence.
///
///   • L'ORDRE DES CHAMPS N'EST PAS CELUI DE LA DOCUMENTATION.
///     Les champs sont réordonnés par taille décroissante (8 octets,
///     puis 4, puis 2, puis 1). Pour NAMED_VALUE_FLOAT la doc liste
///     (time, name, value) mais le fil transporte (time, value, name).
///     Raison : l'alignement mémoire sur les microcontrôleurs.
///
///   • LA CHARGE UTILE EST TRONQUÉE.
///     Les octets nuls de fin sont supprimés. Un nom de 7 caractères
///     dans un champ de 10 fait gagner 3 octets — sur une liaison radio
///     à 57 600 bauds, ça compte.
///
///   • LE CRC INCLUT UN OCTET QUI N'EST PAS DANS LA TRAME.
///     Le « CRC_EXTRA » est une empreinte de la DÉFINITION du message,
///     calculée à la génération. Si l'émetteur et le récepteur n'ont pas
///     la même version du protocole, les CRC divergent et les trames sont
///     rejetées — au lieu d'être interprétées de travers. C'est une
///     protection contre le pire scénario : deux machines qui se
///     comprennent mal en croyant se comprendre.
///
/// VÉRIFICATION : les octets produits par cette classe ont été comparés
/// un par un à ceux de `pymavlink`, l'implémentation de référence. Les
/// vecteurs de test sont dans test/test_mavlink_encoder.cpp.
class MavlinkEncoder {
public:
    /// Taille maximale d'une trame v2 sans signature.
    static constexpr std::size_t kMaxFrame = 280;

    MavlinkEncoder(std::uint8_t system_id, std::uint8_t component_id);

    /// Télémétrie continue : une valeur nommée.
    /// @param name  10 caractères maximum, tronqué au-delà
    /// @return      nombre d'octets écrits dans `out`, 0 si échec
    std::size_t encode_named_value_float(std::uint32_t time_boot_ms,
                                         const char* name, float value,
                                         std::uint8_t* out, std::size_t capacity);

    /// Événement : un message texte affiché au sol.
    /// @param severity  0 = urgence … 4 = avertissement … 6 = information
    /// @param text      50 caractères maximum
    std::size_t encode_statustext(std::uint8_t severity, const char* text,
                                  std::uint8_t* out, std::size_t capacity);

    /// Signe de vie. Sans lui, aucune station sol ne montrera le véhicule.
    ///
    /// Les deux paramètres comptent plus qu'il n'y paraît : une station
    /// sol les lit pour décider CE QU'ELLE VOIT. Annoncer MAV_TYPE_GCS (6)
    /// revient à dire « je suis une autre station sol » — et QGroundControl
    /// affichera alors « Requires a connected vehicle », parce qu'aucun
    /// véhicule ne s'est présenté.
    ///
    /// @param type       MAV_TYPE — 7 = dirigeable, 2 = quadricoptère
    /// @param autopilot  MAV_AUTOPILOT — 0 = générique, 8 = pas un pilote
    std::size_t encode_heartbeat(std::uint8_t* out, std::size_t capacity,
                                 std::uint8_t type = 7, std::uint8_t autopilot = 0);

    std::uint8_t sequence() const { return seq_; }

private:
    std::size_t frame(std::uint32_t msgid, std::uint8_t crc_extra,
                      const std::uint8_t* payload, std::size_t payload_len,
                      std::uint8_t* out, std::size_t capacity);

    std::uint8_t system_id_;
    std::uint8_t component_id_;
    std::uint8_t seq_;
};

}  // namespace core
