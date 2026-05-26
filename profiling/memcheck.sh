cd build || exit 1
if make -j hai819i_moteur_jeux_debug; then
    cd .. || exit 1
    valgrind \
        --tool=memcheck \
        --leak-check=full \
        --show-leak-kinds=all \
        --track-origins=yes \
        --error-exitcode=1 \
        --log-file=valgrind_memcheck.log \
        ./build/hai819i_moteur_jeux_debug
else
    cd .. || exit 1
    exit 1
fi