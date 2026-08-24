# ─────────────────────────────────────────────────────────────
#  Tests unitaires — GoogleTest
# ─────────────────────────────────────────────────────────────
#
# FetchContent télécharge GoogleTest à la configuration.
# Tu n'as donc rien à installer à la main, et la CI non plus :
# n'importe qui clone le dépôt et `cmake -B build` suffit.
#
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2          # version figée : un build reproductible
)                                    # ne dépend jamais de "la dernière version"

FetchContent_MakeAvailable(googletest)

add_executable(test_core
    test_gas_monitor.cpp
)

target_link_libraries(test_core
    PRIVATE
        core            # le code qu'on teste
        GTest::gtest_main   # fournit un main() prêt à l'emploi
)

# Enregistre chaque TEST() auprès de CTest, pour pouvoir lancer
# la suite entière avec une seule commande : ctest
include(GoogleTest)
gtest_discover_tests(test_core)
