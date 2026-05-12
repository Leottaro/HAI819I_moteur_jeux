cd build || exit 1
if make -j hai819i_moteur_jeux_opt; then
    cd .. || exit 1
    valgrind \
        --tool=cachegrind \
        --branch-sim=yes \
        --cachegrind-out-file=valgrind_cachegrind.out \
        ./build/hai819i_moteur_jeux_opt
    cg_annotate valgrind_cachegrind.out
else
    cd .. || exit 1
    exit 1
fi