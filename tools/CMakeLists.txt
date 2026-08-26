# Outils de mise au point — pas destinés à la carte.
#
# Les sockets POSIX n'existent pas sous Windows natif. On ne construit
# donc cet outil que sur les systèmes de type UNIX — ce qui inclut WSL.
# Un projet portable ne suppose jamais que tout le monde a le même
# système : il vérifie, et s'adapte.

if(UNIX)
    add_executable(qgc_stream qgc_stream.cpp)
    target_link_libraries(qgc_stream PRIVATE core)
else()
    message(STATUS "qgc_stream ignore : sockets POSIX indisponibles sur cette plateforme")
endif()
