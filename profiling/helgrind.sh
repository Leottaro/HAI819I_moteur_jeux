cd build || exit 1
if make -j hai819i_moteur_jeux_debug; then
    cd .. || exit 1
    valgrind \
        --tool=helgrind \
        --history-level=full \
        --log-file=valgrind_helgrind.log \
        -s \
        ./build/hai819i_moteur_jeux_debug
else
    cd .. || exit 1
    exit 1
fi