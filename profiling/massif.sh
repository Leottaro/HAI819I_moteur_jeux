cd build || exit 1
if make -j hai819i_moteur_jeux_debug; then
    cd .. || exit 1
    valgrind \
        --tool=massif \
        --pages-as-heap=yes \
        --massif-out-file=valgrind_massif.out \
        ./build/hai819i_moteur_jeux_debug
    # ms_print valgrind_massif.out | less
    # or
    massif-visualizer valgrind_massif.out
else
    cd .. || exit 1
    exit 1
fi