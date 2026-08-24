#include <gtest/gtest.h>

#include "core/GasMonitor.hpp"

using core::AlertState;
using core::GasMonitor;

// ─────────────────────────────────────────────────────────────
//  Anatomie d'un test
// ─────────────────────────────────────────────────────────────
//
//   TEST(NomDuGroupe, ce_que_le_test_prouve) { ... }
//
// Le second nom se lit comme une phrase. « GasMonitor demarre_en_normal »
// est une AFFIRMATION sur le comportement. Quand la CI échouera dans six
// mois, tu liras ce nom avant de lire le code — il doit suffire.
//
// Structure interne, toujours la même (Arrange / Act / Assert) :
//   1. on prépare l'objet
//   2. on agit
//   3. on vérifie
//
// EXPECT_EQ  → si ça rate, le test continue (tu vois toutes les erreurs)
// ASSERT_EQ  → si ça rate, le test s'arrête net (quand la suite n'a plus de sens)

// ══ Les deux tests que je te donne en exemple ═════════════════

TEST(GasMonitor, demarre_en_etat_normal) {
    GasMonitor monitor(1000.0f, 800.0f);

    EXPECT_EQ(monitor.state(), AlertState::Normal);
}

TEST(GasMonitor, bascule_en_alerte_au_dessus_du_seuil_haut) {
    GasMonitor monitor(1000.0f, 800.0f);

    const AlertState result = monitor.update(1500.0f);

    EXPECT_EQ(result, AlertState::Alert);
    EXPECT_EQ(monitor.state(), AlertState::Alert);   // et l'état persiste
}

// ══ À TOI D'ÉCRIRE LES SUIVANTS ═══════════════════════════════
//
// Retire le préfixe DISABLED_ de chaque test au fur et à mesure que tu
// l'implémentes. (GoogleTest saute les tests ainsi préfixés et te le
// signale — pratique pour garder sa liste de choses à faire dans le
// code lui-même plutôt que dans un carnet à côté.)

// Reste en Normal tant qu'on n'a pas atteint le seuil haut.
TEST(GasMonitor, DISABLED_reste_normal_sous_le_seuil_haut) {
    // Indice : 999 ppm avec un seuil à 1000, ça ne doit RIEN déclencher.
    FAIL() << "à implémenter";
}

// LE test important : c'est lui qui prouve l'hystérésis.
// Une fois en alerte, redescendre dans la bande morte (entre falling
// et rising) ne doit PAS calmer l'alerte.
TEST(GasMonitor, DISABLED_reste_en_alerte_dans_la_bande_morte) {
    // Scénario : monte à 1500 (→ Alert), puis redescend à 900.
    // 900 est sous le seuil haut mais au-dessus du seuil bas.
    // Attendu : toujours Alert.
    FAIL() << "à implémenter";
}

// Le retour au calme, une fois vraiment redescendu.
TEST(GasMonitor, DISABLED_revient_en_normal_sous_le_seuil_bas) {
    FAIL() << "à implémenter";
}

// Les bornes exactes. C'est là que vivent la moitié des bugs réels :
// « au-dessus » et « au-dessus ou égal » ne sont pas la même chose,
// et personne ne s'en aperçoit avant que ça compte.
TEST(GasMonitor, DISABLED_le_seuil_exact_declenche_l_alerte) {
    // Que doit faire update(1000.0f) avec un seuil à 1000 ?
    // La doc de l'en-tête dit « atteint ou dépasse ». Fige-le ici.
    FAIL() << "à implémenter";
}

// Le test qui raconte le vrai problème : un capteur qui oscille.
TEST(GasMonitor, DISABLED_ne_clignote_pas_quand_le_capteur_oscille) {
    // Envoie 850, 950, 870, 940, 880... (tout dans la bande morte)
    // en partant de Normal. L'état ne doit jamais bouger.
    //
    // Puis refais la même oscillation en partant d'Alert : l'état ne
    // doit pas bouger non plus. Même séquence d'entrée, deux résultats
    // différents — c'est exactement ce qu'on veut, et c'est ce qu'un
    // seuil unique est incapable de faire.
    FAIL() << "à implémenter";
}

// ══ POUR ALLER PLUS LOIN ══════════════════════════════════════
//
// Quand les six passent au vert, pose-toi ces questions et écris le
// test correspondant à ta réponse :
//
//   • que se passe-t-il si on construit avec rising < falling ?
//   • et si le capteur renvoie une valeur négative, physiquement absurde ?
//   • et un NaN (ça arrive : division par zéro dans une calibration) ?
//
// Ce dernier cas est un piège classique en C++ : toute comparaison avec
// NaN est fausse, y compris NaN == NaN. Regarde ce que fait ton code,
// décide ce qu'il DEVRAIT faire, et écris le test qui fige ta décision.
// En vol, un capteur qui renvoie NaN n'est pas une hypothèse d'école.
