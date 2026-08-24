# ─────────────────────────────────────────────────────────────
#  gas-payload-node — configuration de compilation principale
# ─────────────────────────────────────────────────────────────
#
# CMake ne compile pas : il GÉNÈRE les fichiers de compilation
# (des Makefile sous Linux) à partir de cette description.
# D'où les deux commandes distinctes :
#     cmake -B build      → génère
#     cmake --build build → compile
#
cmake_minimum_required(VERSION 3.16)

project(gas_payload_node
        VERSION 0.1.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)      # du C++ standard, pas des extensions GNU

# Les avertissements traités sérieusement. En embarqué, un warning
# ignoré est un bug qui attend son tour.
add_compile_options(-Wall -Wextra -Wpedantic)

# ── La bibliothèque « core » ────────────────────────────────
#
# Point crucial pour la suite : cette cible ne dépend d'AUCUN
# matériel. Elle compile pour ton PC comme pour un STM32.
# C'est ce qui la rendra testable en intégration continue.
#
add_library(core
    src/core/GasMonitor.cpp
)

target_include_directories(core PUBLIC include)

# ── Les tests ───────────────────────────────────────────────
#
# Optionnels : la carte n'a pas besoin d'embarquer GoogleTest.
# On les active sur PC et dans la CI, jamais sur la cible.
#
option(BUILD_TESTS "Compiler les tests unitaires" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
