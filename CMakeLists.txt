# ─────────────────────────────────────────────────────────────
#  gas-payload-node — configuration de compilation principale
# ─────────────────────────────────────────────────────────────
cmake_minimum_required(VERSION 3.16)

project(gas_payload_node
        VERSION 0.2.0
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ── La bibliothèque « core » ────────────────────────────────
#
# LA RÈGLE, ET ELLE N'A PAS D'EXCEPTION :
#   aucun fichier de cette cible n'inclut d'en-tête matériel.
#
# C'est cette règle qui permet à `core` de compiler sur ton PC, sur un
# serveur GitHub et sur un STM32 — le même code, sans une ligne de
# différence. Le jour où tu es tenté d'ajouter ici un fichier qui
# inclut un en-tête de fabricant, c'est le signe qu'il appartient à
# `hal/`, pas à `core/`.
#
add_library(core
    src/core/GasMonitor.cpp
    src/core/PayloadService.cpp
    src/core/MavlinkEncoder.cpp
    src/core/TelemetryReporter.cpp
)

target_include_directories(core PUBLIC include)

# ── La couche matérielle ────────────────────────────────────
#
# Le code du driver n'a PAS été supprimé — il a été mis derrière un
# interrupteur. Par défaut il n'est pas construit, donc il ne gêne ni
# ton PC ni la CI. Quand tu viseras la carte, en séance 6 :
#
#     cmake -B build-stm32 -DBUILD_HAL_STM32=ON \
#           -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi.cmake
#
# C'est la traduction en CMake de la frontière qu'on vient de tracer
# dans le code.
#
option(BUILD_HAL_STM32 "Construire le driver STM32 (nécessite la chaîne ARM)" OFF)

if(BUILD_HAL_STM32)
    add_library(hal_stm32
        src/hal/stm32/Mq135Driver.cpp
    )
    target_link_libraries(hal_stm32 PUBLIC core)
endif()

# ── Les tests ───────────────────────────────────────────────
option(BUILD_TESTS "Compiler les tests unitaires" ON)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

# ── La démonstration ────────────────────────────────────────
#
# On vérifie que le dossier existe avant de l'inclure. Sans ce garde-fou,
# un projet où `demo/` a été supprimé refuse de se configurer — et le
# message d'erreur ne dit pas quoi faire. Un CMakeLists tolérant aux
# absences est un CMakeLists qu'on peut cloner partiellement.
#
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/demo/CMakeLists.txt)
    add_subdirectory(demo)
endif()

# ── Outils de mise au point ─────────────────────────────────
add_subdirectory(tools)
