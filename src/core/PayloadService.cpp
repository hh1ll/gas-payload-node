#include "core/PayloadService.hpp"

namespace core {

PayloadService::PayloadService(IGasSensor& sensor, GasMonitor monitor,
                               int failures_before_lost)
    : sensor_(sensor),
      monitor_(monitor),
      failures_before_lost_(failures_before_lost),
      consecutive_failures_(0),
      health_(ServiceHealth::Ok),
      last_valid_() {}

// ─────────────────────────────────────────────────────────────
//  À TOI DE JOUER
// ─────────────────────────────────────────────────────────────
//
// Comme en séance 1 : le squelette compile, la logique est absente.
// Lance les tests d'abord, vois-les rouges, puis implémente.
//
// LES RÈGLES
//
//   1. Appelle sensor_.read(sample).
//
//   2. La lecture est un SUCCÈS si read() a renvoyé true ET que
//      sample.health vaut SensorHealth::Ok. Les deux conditions.
//
//   3. En cas de succès :
//        • remettre consecutive_failures_ à zéro
//        • health_ = Ok
//        • mémoriser l'échantillon dans last_valid_
//        • le transmettre à monitor_.update(...)
//
//   4. En cas d'échec :
//        • incrémenter consecutive_failures_
//        • health_ = Lost si le compteur atteint failures_before_lost_,
//          sinon Degraded
//        • NE PAS toucher à last_valid_
//        • NE PAS appeler monitor_.update(...)
//
// ─────────────────────────────────────────────────────────────
//  LA RÈGLE 4 EST LA PLUS IMPORTANTE DU PROJET
// ─────────────────────────────────────────────────────────────
//
// Pourquoi ne pas appeler monitor_.update() quand la lecture échoue ?
//
// Parce qu'il faudrait bien lui passer QUELQUE CHOSE. Et la seule valeur
// disponible serait 0, ou une valeur par défaut. Or 0 ppm est sous le
// seuil bas — donc GasMonitor conclurait « plus de gaz » et éteindrait
// l'alerte.
//
// Autrement dit : le drone survole une fuite de méthane, le capteur
// tombe en panne, et l'alarme s'éteint toute seule. L'opérateur au sol
// en déduit que la fuite est colmatée.
//
// Un capteur muet ne dit pas « tout va bien ». Il ne dit RIEN. Confondre
// les deux est une faute de conception classique en systèmes critiques,
// et c'est exactement le genre de raisonnement que HyLight cherche quand
// l'annonce parle de « robustness and reliability ».
//
// L'alerte doit donc PERSISTER, accompagnée d'une santé dégradée qui
// dit au sol : « je maintiens ma dernière conclusion, mais je ne vois
// plus rien. »

void PayloadService::poll() {

    // ───── ÉTAPE 1 ─────────────────────────────────────────
    // On crée une variable vide pour recevoir la mesure.
    // Le capteur la remplira. `GasSample` est la structure définie
    // dans include/core/GasSample.hpp.

    GasSample sample;


    // ───── ÉTAPE 2 ─────────────────────────────────────────
    // On demande une mesure, et on juge si elle est exploitable.
    //
    // La ligne ci-dessous est VOLONTAIREMENT INCOMPLÈTE : elle ne
    // teste qu'une des deux conditions. Il manque la seconde.
    //
    //   condition 1 : sensor_.read(sample) a renvoyé true
    //                 → on a réussi à parler au capteur
    //   condition 2 : sample.health vaut SensorHealth::Ok
    //                 → le capteur ne se déclare pas en défaut
    //
    // Ajoute la seconde avec  &&.

    const bool lecture_ok = sensor_.read(sample) && sample.health == SensorHealth::Ok;


    if (lecture_ok) {
        // ───── ÉTAPE 3 : la lecture a réussi ───────────────
        //
        // Quatre choses à faire, une ligne chacune :
        //
        //   3a. remettre  consecutive_failures_  à 0
        consecutive_failures_ = 0;
        //   3b. mettre    health_  à  ServiceHealth::Ok
        health_ = ServiceHealth::Ok;
        //   3c. mémoriser la mesure :  last_valid_ = sample;
        last_valid_ = sample;
        //   3d. transmettre la concentration à la décision :
        //          monitor_.update( ... );
        monitor_.update(sample.concentration_ppm);
        //       (que faut-il lui passer ? regarde les champs de
        //        `sample` dans GasSample.hpp)

        // ton code ici

    } else {
        // ───── ÉTAPE 4 : la lecture a échoué ───────────────
        //
        //   4a. augmenter  consecutive_failures_  de 1
        //       (en C++ :  ++consecutive_failures_;  )
        ++consecutive_failures_;
        //
        //   4b. choisir la santé :
        //       si  consecutive_failures_  a atteint
        //       failures_before_lost_  →  ServiceHealth::Lost
        //       sinon                  →  ServiceHealth::Degraded
        if (consecutive_failures_ >= failures_before_lost_) {
            health_ = ServiceHealth::Lost;
        } else {
            health_ = ServiceHealth::Degraded;
        }
        //
        //       Écris-le avec un if / else tout simple, c'est
        //       parfait. (Il existe une écriture plus courte avec
        //       ? :  mais elle n'apporte rien ici.)
        //
        //   4c. ET C'EST TOUT. Surtout, n'appelle PAS
        //       monter_.update() ici — c'est la règle qui empêche
        //       une panne capteur d'éteindre l'alerte.
        

        // ton code ici
    }
}

AlertState PayloadService::alert() const {
    return monitor_.state();
}

ServiceHealth PayloadService::health() const {
    return health_;
}

const GasSample& PayloadService::last_valid_sample() const {
    return last_valid_;
}

int PayloadService::consecutive_failures() const {
    return consecutive_failures_;
}

}  // namespace core