#include <gtest/gtest.h>

#include "FakeGasSensor.hpp"
#include "FakeTelemetrySink.hpp"
#include "core/MavlinkEncoder.hpp"
#include "core/PayloadService.hpp"
#include "core/TelemetryReporter.hpp"

using core::AlertState;
using core::GasMonitor;
using core::MavlinkEncoder;
using core::PayloadService;
using core::TelemetryReporter;

namespace {

/// Le montage complet de la chaîne, en quatre lignes et sans matériel :
/// faux capteur → service → rapporteur → fausse radio.
struct Rig {
    FakeGasSensor     sensor;
    FakeTelemetrySink sink;
    MavlinkEncoder    encoder{1, 25};

    PayloadService service() { return PayloadService(sensor, GasMonitor(1000.0f, 800.0f), 3); }
};

}  // namespace

// ══ Les deux tests que je te donne ════════════════════════════

TEST(TelemetryReporter, emet_la_concentration_a_chaque_cycle) {
    Rig rig;
    rig.sensor.push_many(3, 500.0f);

    PayloadService service = rig.service();
    TelemetryReporter reporter(rig.sink, rig.encoder);

    for (int i = 0; i < 3; ++i) {
        service.poll();
        reporter.report(service, static_cast<std::uint32_t>(i * 100));
    }

    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kNamedValueFloat), 3u);
}

TEST(TelemetryReporter, annonce_l_etat_au_premier_cycle) {
    Rig rig;
    rig.sensor.push(500.0f);

    PayloadService service = rig.service();
    TelemetryReporter reporter(rig.sink, rig.encoder);

    service.poll();
    reporter.report(service, 0);

    // Même en état normal, le premier cycle doit dire quelque chose :
    // sinon l'opérateur ne sait pas si la charge utile est vivante.
    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kStatustext), 1u);
}

// ══ À TOI D'ÉCRIRE LES SUIVANTS ═══════════════════════════════

// Tant que rien ne change, aucun STATUSTEXT supplémentaire.
TEST(TelemetryReporter, ne_repete_pas_l_etat_inchange) {
    // Cinq cycles à 500 ppm, tous en état normal.
    // Attendu : 5 NAMED_VALUE_FLOAT, mais UN SEUL STATUSTEXT (le premier).
    Rig rig;
    rig.sensor.push_many(5, 500.0f);

    PayloadService service = rig.service();
    TelemetryReporter reporter(rig.sink, rig.encoder);

    for (int i = 0; i < 5;i++) {
        service.poll();
        reporter.report(service, static_cast<std::uint32_t>(i * 100));
    }
    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kNamedValueFloat), 5u);
    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kStatustext), 1u);
}

// Le passage en alerte doit être annoncé.
TEST(TelemetryReporter, annonce_le_passage_en_alerte) {
    // Scénario : 500 ppm, puis 1500 ppm.
    // Attendu : 2 STATUSTEXT au total — l'état initial, puis le changement.
    Rig rig;
    rig.sensor.push(500.0f);
    rig.sensor.push(1500.0f);

    PayloadService service = rig.service();
    TelemetryReporter reporter(rig.sink, rig.encoder);

    for (int i = 0; i < 2; ++i) {
        service.poll();
        reporter.report(service, static_cast<std::uint32_t>(i * 100));
    }
    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kStatustext), 2u);
}

// Le retour au calme aussi.
TEST(TelemetryReporter, annonce_le_retour_a_la_normale) {
    // 500 → 1500 → 500 : trois STATUSTEXT.
    // Attention au seuil bas : il faut vraiment redescendre sous 800 ppm
    // pour que l'hystérésis relâche l'alerte. Tu as écrit ce test-là en
    // séance 1, sous un autre nom.
    Rig rig;
    rig.sensor.push(500.0f);
    rig.sensor.push(1500.0f);
    rig.sensor.push(500.0f);
    PayloadService service = rig.service();
    TelemetryReporter reporter(rig.sink, rig.encoder);

    for (int i = 0; i < 3; ++i) {
        service.poll();
        reporter.report(service, static_cast<std::uint32_t>(i * 100));
    }
    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kStatustext), 3u);
}

// ══════════════════════════════════════════════════════════════
//  LE TEST QUI RELIE TOUTE LA CHAÎNE
// ════════════════════════════════════════
//
// Celui-ci traverse les trois séances d'un coup : hystérésis (1),
// gestion de panne (3), et régime d'émission (4).
//
// Scénario : l'alerte se déclenche, puis le capteur meurt.
//
// Attendu :
//   • la concentration continue d'être émise (la dernière valide, figée)
//   • AUCUN nouveau STATUSTEXT — l'état d'alerte n'a pas changé, il a
//     seulement cessé d'être rafraîchi
//
// Autrement dit : la panne ne doit ni éteindre l'alerte, ni déclencher
// une avalanche de messages. Le sol garde la bonne conclusion.
TEST(TelemetryReporter, une_panne_ne_declenche_pas_de_nouveau_message) {
    Rig rig;
    rig.sensor.push(500.0f);          // Normal
    rig.sensor.push(1500.0f);         // Alerte
    rig.sensor.push_read_failure();   // Panne
    rig.sensor.push_read_failure();   // Panne
    rig.sensor.push_read_failure();   // Panne

    PayloadService service = rig.service();
    TelemetryReporter reporter(rig.sink, rig.encoder);

    for (int i = 0; i < 5; ++i) {
        service.poll();
        reporter.report(service, static_cast<std::uint32_t>(i * 100));
    }

    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kNamedValueFloat), 5u);
    EXPECT_EQ(rig.sink.count_of(FakeTelemetrySink::kStatustext), 2u);
}

// ══ POUR ALLER PLUS LOIN ══════════════════════════════════════
//
// • Faudrait-il émettre un STATUSTEXT quand la SANTÉ change
//   (Ok → Degraded → Lost) ? Que verrait l'opérateur au sol s'il ne
//   savait jamais que le capteur est muet ?
// • `sink_.send()` peut renvoyer false — liaison saturée ou coupée.
//   Aujourd'hui on ignore ce retour. Que devrait-on en faire :
//   réessayer, compter, abandonner ? (FakeTelemetrySink::fail_next_send()
//   existe pour que tu puisses tester ta réponse.)
