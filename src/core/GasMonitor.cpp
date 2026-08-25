#include "core/GasMonitor.hpp"

namespace core {

    //ce code nous permet de créer un objet GasMonitor avec des seuils de déclenchement et de retour au calme, 
    //et de mettre à jour l'état d'alerte en fonction de la concentration mesurée. 
    //Il implémente également une hystérésis pour éviter les changements d'état fréquents lorsque la concentration oscille 
    //autour des seuils.

GasMonitor::GasMonitor(float rising_ppm, float falling_ppm)
    : rising_ppm_(rising_ppm),
      falling_ppm_(falling_ppm),
      state_(AlertState::Normal) {
    
}

AlertState GasMonitor::update(float concentration_ppm) {
    //ici le but est d'instaurer une hystérésis, donc on ne change l'état que si on franchit les seuils
    if (concentration_ppm <= falling_ppm_) {        //détermine la limite inférieure de la bande morte
        state_ = AlertState::Normal;
    } 
    else if (concentration_ppm >= rising_ppm_) {    //détermine la limite supérieure de la bande morte
        state_ = AlertState::Alert;
    }

    return state_;
}

AlertState GasMonitor::state() const {
    return state_;
}

} 
