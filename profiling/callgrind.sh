cd build || exit 1
if make -j hai819i_moteur_jeux_opt; then
    cd .. || exit 1
    valgrind \
        --tool=callgrind \
        --callgrind-out-file=valgrind_callgrind.out \
        --collect-jumps=yes \
        --simulate-cache=yes \
        ./build/hai819i_moteur_jeux_opt
    kcachegrind valgrind_callgrind.out
else
    cd .. || exit 1
    exit 1
fi