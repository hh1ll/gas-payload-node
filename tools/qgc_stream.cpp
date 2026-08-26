// ─────────────────────────────────────────────────────────────
//  qgc_stream — envoie la télémétrie vers QGroundControl
// ─────────────────────────────────────────────────────────────
//
// Ce programme monte la chaîne complète et l'envoie sur une socket UDP,
// vers le port 14550 — celui que QGroundControl écoute par défaut.
//
// REGARDE BIEN CE QU'IL CONTIENT.
//
// Il utilise `PayloadService` et `TelemetryReporter` sans en modifier
// une seule ligne. Il se contente de leur fournir DEUX IMPLÉMENTATIONS
// DIFFÉRENTES des interfaces :
//
//     IGasSensor     → SimulatedGasSensor  (un panache calculé)
//     ITelemetrySink → UdpSink             (une socket réseau)
//
// Dans les tests, c'étaient FakeGasSensor et FakeTelemetrySink. Sur la
// carte, ce seront Mq135Driver et UartSink. Le code du milieu ne change
// jamais, et ne sait même pas qu'il y a plusieurs mondes.
//
// C'est ça, le bénéfice concret des interfaces de la séance 3. Pas une
// élégance théorique : trois usages réels du même code.

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "core/MavlinkEncoder.hpp"
#include "core/PayloadService.hpp"
#include "core/TelemetryReporter.hpp"

using namespace core;

namespace {

/// Un capteur qui simule un survol de fuite : ligne de base, panache
/// gaussien, bruit de mesure.
class SimulatedGasSensor : public IGasSensor {
public:
    bool read(GasSample& out) override {
        const float t = static_cast<float>(tick_++) * 0.2f;   // 5 Hz

        const float d     = t - 12.0f;
        const float plume = 700.0f * std::exp(-(d * d) / 12.0f);
        const float noise = 120.0f * std::sin(t * 5.1f) + 60.0f * std::sin(t * 2.7f);

        out.concentration_ppm = 400.0f + plume + noise;
        out.timestamp_us      = static_cast<std::uint64_t>(t * 1e6f);
        out.health            = SensorHealth::Ok;

        // Pendant le survol de la fuite, on simule une panne capteur de
        // 4 secondes — pour voir au sol que l'alerte NE s'éteint PAS.
        const int sec = static_cast<int>(t);
        if (sec >= 14 && sec < 18) return false;

        return true;
    }

private:
    int tick_ = 0;
};

/// Une radio qui est en fait une socket UDP.
class UdpSink : public ITelemetrySink {
public:
    UdpSink(const char* host, std::uint16_t port) {
        fd_ = ::socket(AF_INET, SOCK_DGRAM, 0);
        std::memset(&addr_, 0, sizeof(addr_));
        addr_.sin_family = AF_INET;
        addr_.sin_port   = htons(port);
        ::inet_pton(AF_INET, host, &addr_.sin_addr);
    }

    ~UdpSink() override { if (fd_ >= 0) ::close(fd_); }

    bool send(const std::uint8_t* data, std::size_t length) override {
        if (fd_ < 0) return false;
        const ssize_t n = ::sendto(fd_, data, length, 0,
                                   reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_));
        return n == static_cast<ssize_t>(length);
    }

private:
    int         fd_ = -1;
    sockaddr_in addr_{};
};

void sleep_ms(int ms) {
    timespec ts{ms / 1000, (ms % 1000) * 1000000L};
    ::nanosleep(&ts, nullptr);
}

const char* label(ServiceHealth h) {
    switch (h) {
        case ServiceHealth::Ok:       return "ok      ";
        case ServiceHealth::Degraded: return "degrade ";
        case ServiceHealth::Lost:     return "PERDU   ";
    }
    return "?";
}

}  // namespace

int main(int argc, char** argv) {
    const char*         host = (argc > 1) ? argv[1] : "127.0.0.1";
    const std::uint16_t port = (argc > 2) ? static_cast<std::uint16_t>(std::atoi(argv[2]))
                                          : 14550;

    SimulatedGasSensor sensor;
    UdpSink            sink(host, port);
    // Identifiant de composant 1 = MAV_COMP_ID_AUTOPILOT1. QGroundControl
    // ne crée un véhicule qu'à partir du composant pilote ; un composant
    // de charge utile (25) serait rattaché à un véhicule existant... qui
    // n'existe pas ici. On se présente donc comme l'aéronef lui-même.
    MavlinkEncoder     encoder(1, 1);

    PayloadService    service(sensor, GasMonitor(1000.0f, 800.0f), 3);
    TelemetryReporter reporter(sink, encoder);

    std::printf("\n  Emission MAVLink vers %s:%u\n", host, port);
    std::printf("  Ouvre QGroundControl, puis : Analyze > MAVLink Inspector\n");
    std::printf("  Cherche le message NAMED_VALUE_FLOAT, champ GAS_PPM.\n");
    std::printf("  Ctrl+C pour arreter.\n\n");

    int cycle = 0;
    for (;;) {
        // Battement de coeur a 1 Hz : sans lui, QGC n'affiche aucun vehicule.
        if (cycle % 5 == 0) reporter.send_heartbeat();

        service.poll();
        reporter.report(service, static_cast<std::uint32_t>(cycle * 200));

        if (cycle % 5 == 0) {
            std::printf("  t=%5.1fs  %7.0f ppm   %s  %s\n",
                        cycle * 0.2,
                        static_cast<double>(service.last_valid_sample().concentration_ppm),
                        label(service.health()),
                        service.alert() == AlertState::Alert ? "<<< ALERTE" : "");
            std::fflush(stdout);
        }

        ++cycle;
        sleep_ms(200);                       // 5 Hz
    }
}
