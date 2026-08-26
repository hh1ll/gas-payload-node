#pragma once

#include <cstddef>
#include <cstdint>

namespace core {

/// Où partent les octets de télémétrie.
///
/// Même motif qu'`IGasSensor`, et ce n'est pas un hasard : c'est LE motif
/// de ce projet. À chaque fois que ta logique doit toucher le monde
/// extérieur — un capteur, une radio, une horloge, une carte SD — tu
/// places une interface à la frontière.
///
/// Implémentations réelles : un port série vers la radio, une socket UDP
/// vers QGroundControl, un fichier de log.
/// Implémentation de test : un sac qui garde les octets en mémoire, pour
/// que tu puisses vérifier exactement ce qui a été émis.
///
/// Remarque : cette interface parle en OCTETS, pas en « messages ». Elle
/// ne sait pas ce qu'est MAVLink, et c'est volontaire — une radio n'a pas
/// à connaître le protocole qu'elle transporte. Une couche, une
/// responsabilité.
class ITelemetrySink {
public:
    virtual ~ITelemetrySink() = default;

    /// Émet un bloc d'octets.
    /// @return false si l'émission a échoué (tampon plein, liaison coupée)
    virtual bool send(const std::uint8_t* data, std::size_t length) = 0;
};

}  // namespace core
