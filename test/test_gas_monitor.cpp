#include <gtest/gtest.h>

#include "core/GasMonitor.hpp"

using core::AlertState;
using core::GasMonitor;



// EXPECT_EQ  → si ça rate, le test continue
// ASSERT_EQ  → si ça rate, le test s'arrête net


TEST(GasMonitor, demarre_en_etat_normal) {
    GasMonitor monitor(1000.0f, 800.0f);

    EXPECT_EQ(monitor.state(), AlertState::Normal);
}

TEST(GasMonitor, bascule_en_alerte_au_dessus_du_seuil_haut) {
    GasMonitor monitor(1000.0f, 800.0f);

    const AlertState result = monitor.update(1500.0f);

    EXPECT_EQ(result, AlertState::Alert);
    EXPECT_EQ(monitor.state(), AlertState::Alert);   
}


TEST(GasMonitor, reste_normal_sous_le_seuil_haut) {
    GasMonitor monitor(1000.0f, 800.0f);

    const AlertState result = monitor.update(999.0f);

    EXPECT_EQ(monitor.state(), AlertState::Normal);
}


TEST(GasMonitor, reste_en_alerte_dans_la_bande_morte) {
    GasMonitor monitor(1000.0f, 800.0f);
    monitor.update(1500.0f);  // Passer en alerte
    const AlertState result = monitor.update(900.0f);  // Redescendre dans la bande morte

    EXPECT_EQ(result, AlertState::Alert);
    EXPECT_EQ(monitor.state(), AlertState::Alert);
}


TEST(GasMonitor, revient_en_normal_sous_le_seuil_bas) {
    GasMonitor monitor(1000.0f, 800.0f);
    monitor.update(1500.0f);  // Passer en alerte
    const AlertState result = monitor.update(799.0f);  // Redescendre sous le seuil bas

    EXPECT_EQ(result, AlertState::Normal);
    EXPECT_EQ(monitor.state(), AlertState::Normal);
}


TEST(GasMonitor, le_seuil_exact_declenche_l_alerte) {
    GasMonitor monitor(1000.0f, 800.0f);
    const AlertState result = monitor.update(1000.0f);

    EXPECT_EQ(result, AlertState::Alert);
    EXPECT_EQ(monitor.state(), AlertState::Alert);
}


TEST(GasMonitor, ne_clignote_pas_quand_le_capteur_oscille) {
    GasMonitor monitor(1000.0f, 800.0f);
    monitor.update(1500.0f);  // Passer en alerte

    // Osciller dans la bande morte
    const AlertState result1 = monitor.update(850.0f);
    const AlertState result2 = monitor.update(950.0f);
    const AlertState result3 = monitor.update(870.0f);
    const AlertState result4 = monitor.update(940.0f);
    const AlertState result5 = monitor.update(880.0f);

    EXPECT_EQ(result1, AlertState::Alert);
    EXPECT_EQ(result2, AlertState::Alert);
    EXPECT_EQ(result3, AlertState::Alert);
    EXPECT_EQ(result4, AlertState::Alert);
    EXPECT_EQ(result5, AlertState::Alert);
}


