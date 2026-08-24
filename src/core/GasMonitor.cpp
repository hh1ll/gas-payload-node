#include "core/GasMonitor.hpp"

namespace core {

// ─────────────────────────────────────────────────────────────
//  À TOI DE JOUER
// ─────────────────────────────────────────────────────────────
//
// Le squelette ci-dessous compile, mais la logique est fausse
// exprès : update() ignore l'échantillon et ne bascule jamais.
//
// Lance les tests AVANT d'écrire quoi que ce soit :
//
//     cmake -B build && cmake --build build && ctest --test-dir build
//
// Tu dois voir des tests ROUGES. C'est normal, et c'est même le but :
// un test qui n'a jamais échoué ne prouve rien — il pourrait très bien
// ne rien vérifier du tout. Le voir échouer, c'est tester le test.
//
// Ensuite, implémente jusqu'à ce que tout passe au vert.
//
// Les trois règles à traduire en code :
//   1. on démarre en Normal
//   2. Normal → Alert  quand l'échantillon atteint ou dépasse rising_ppm_
//   3. Alert  → Normal quand l'échantillon atteint ou descend sous falling_ppm_
//   4. entre les deux : on ne change rien
//
// (Oui, il y a quatre points dans une liste de trois règles. La règle 4
//  est celle qu'on oublie toujours, et c'est celle qui fait l'hystérésis.)

GasMonitor::GasMonitor(float rising_ppm, float falling_ppm)
    : rising_ppm_(rising_ppm),
      falling_ppm_(falling_ppm),
      state_(AlertState::Normal) {
    // Question de conception à te poser : que faire si on te passe
    // rising < falling ? Ignorer ? Échanger les deux ? Refuser ?
    // Il n'y a pas de bonne réponse universelle — mais il y a une
    // mauvaise pratique : ne pas décider, et laisser le comportement
    // au hasard. Quoi que tu choisisses, écris le test qui le fige.
}

AlertState GasMonitor::update(float concentration_ppm) {
    (void)concentration_ppm;   // supprime le warning "paramètre inutilisé"
                               // — à retirer dès que tu t'en sers

    // TODO: implémenter les règles ci-dessus.

    return state_;
}

AlertState GasMonitor::state() const {
    return state_;
}

}  // namespace core
