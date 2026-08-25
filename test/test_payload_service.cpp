#include <gtest/gtest.h>

#include "FakeGasSensor.hpp"
#include "core/PayloadService.hpp"

using core::AlertState;
using core::GasMonitor;
using core::PayloadService;
using core::SensorHealth;
using core::ServiceHealth;

namespace {

/// Réglages partagés : seuils 1000/800, capteur déclaré perdu après
/// 3 échecs consécutifs.
PayloadService make_service(FakeGasSensor& sensor) {
    return PayloadService(sensor, GasMonitor(1000.0f, 800.0f), 3);
}

}  // namespace

// ══ Les deux tests que je te donne ════════════════════════════

TEST(PayloadService, transmet_la_mesure_a_la_logique_de_decision) {
    FakeGasSensor sensor;
    sensor.push(1500.0f);              // bien au-dessus du seuil haut

    PayloadService service = make_service(sensor);
    service.poll();

    EXPECT_EQ(service.alert(), AlertState::Alert);
    EXPECT_EQ(service.health(), ServiceHealth::Ok);
    EXPECT_FLOAT_EQ(service.last_valid_sample().concentration_ppm, 1500.0f);
}

TEST(PayloadService, une_lecture_ratee_degrade_sans_perdre) {
    FakeGasSensor sensor;
    sensor.push_read_failure();

    PayloadService service = make_service(sensor);
    service.poll();

    EXPECT_EQ(service.health(), ServiceHealth::Degraded);
    EXPECT_EQ(service.consecutive_failures(), 1);
}

// ══ À TOI D'ÉCRIRE LES SUIVANTS ═══════════════════════════════
//
// Retire DISABLED_ au fur et à mesure.

// Une mesure sous le seuil ne doit pas déclencher d'alerte.
TEST(PayloadService, reste_normal_sous_le_seuil) {
    FakeGasSensor sensor;
    sensor.push(500.0f);              // bien sous le seuil bas

    PayloadService service = make_service(sensor);
    service.poll();

    EXPECT_EQ(service.alert(), AlertState::Normal);
    EXPECT_EQ(service.health(), ServiceHealth::Ok);
    EXPECT_FLOAT_EQ(service.last_valid_sample().concentration_ppm, 500.0f);
}

// Le capteur répond, mais se déclare en défaut. C'est un échec, même si
// read() a renvoyé true — les deux conditions comptent.
TEST(PayloadService, un_capteur_en_defaut_compte_comme_un_echec) {
    // Indice : sensor.push(1500.0f, SensorHealth::Fault);
    // La valeur est plausible, mais le capteur dit lui-même de ne pas
    // s'y fier. On ne la transmet pas à la décision.
    FakeGasSensor sensor;
    sensor.push(1500.0f, SensorHealth::Fault);

    PayloadService service = make_service(sensor);
    service.poll();

    EXPECT_EQ(service.health(), ServiceHealth::Degraded);
    EXPECT_EQ(service.consecutive_failures(), 1);
    EXPECT_FLOAT_EQ(service.last_valid_sample().concentration_ppm, 0.0f);  // valeur par défaut, pas de mesure valide
}

// Trois échecs consécutifs : le capteur est déclaré perdu.
TEST(PayloadService, trois_echecs_consecutifs_perdent_le_capteur) {
    FakeGasSensor sensor;
    sensor.push_read_failure();
    sensor.push_read_failure();
    sensor.push_read_failure();

    PayloadService service = make_service(sensor);
    service.poll();
    service.poll();
    service.poll();

    EXPECT_EQ(service.health(), ServiceHealth::Lost);
    EXPECT_EQ(service.consecutive_failures(), 3);
}

// Une lecture réussie après des échecs remet le compteur à zéro.
TEST(PayloadService, une_lecture_reussie_efface_les_echecs) {
    // Scénario : deux échecs, puis une mesure correcte.
    // Attendu : health == Ok, consecutive_failures() == 0.
    FakeGasSensor sensor;
    sensor.push_read_failure();
    sensor.push_read_failure();
    sensor.push(1500.0f);  // mesure correcte

    PayloadService service = make_service(sensor);
    service.poll();
    service.poll();
    service.poll();

    EXPECT_EQ(service.health(), ServiceHealth::Ok);
    EXPECT_EQ(service.consecutive_failures(), 0);
}

// ══════════════════════════════════════════════════════════════
//  LE TEST QUI COMPTE VRAIMENT
// ══════════════════════════════════════════════════════════════
//
// Celui-là, écris-le en dernier — et retiens-le, parce que c'est
// l'anecdote à raconter en entretien.
//
// Scénario : le drone survole une fuite. L'alerte se déclenche.
// Puis le capteur tombe en panne.
//
// L'alerte doit PERSISTER. Si elle s'éteignait, l'opérateur au sol
// conclurait que la fuite est colmatée — alors qu'en réalité on ne
// voit tout simplement plus rien.
//
// Un capteur muet ne dit pas « tout va bien ». Il ne dit rien.
TEST(PayloadService, une_panne_capteur_n_eteint_jamais_l_alerte) {
     FakeGasSensor sensor;
     sensor.push(1500.0f);          // alerte
     sensor.push_read_failure();    // panne
     sensor.push_read_failure();
     sensor.push_read_failure();

    //
    // Après quatre poll() :
    //   alert()  doit valoir Alert   ← le point crucial
    //   health() doit valoir Lost
    PayloadService service = make_service(sensor);
    service.poll();
    service.poll();
    service.poll();
    service.poll();

    EXPECT_EQ(service.alert(), AlertState::Alert);
    EXPECT_EQ(service.health(), ServiceHealth::Lost);
}

// ══ POUR ALLER PLUS LOIN ══════════════════════════════════════
//
// • Que se passe-t-il avec failures_before_lost = 0 ou négatif ?
// • Faudrait-il un moyen de revenir de Lost à Ok, ou une perte
//   est-elle définitive jusqu'au redémarrage ? Il n'y a pas de bonne
//   réponse universelle — mais il y a une mauvaise pratique : ne pas
//   décider. Choisis, et fige ton choix par un test.
// • `last_valid_sample()` porte un `timestamp_us`. Au bout de combien
//   de temps une mesure figée devient-elle trompeuse ? Répondre à ça
//   demande une abstraction de l'horloge — un `IClock`, exactement sur
//   le même modèle qu'`IGasSensor`. C'est le prolongement naturel.
