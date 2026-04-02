#!/usr/bin/env bash

mode=${1:-opt}

case "$mode" in
    debug)
        target="hai819i_moteur_jeux_debug"
        ;;
    opt)
        target="hai819i_moteur_jeux_opt"
        ;;
    *)
        echo "Usage: $0 [debug|opt]"
        exit 1
        ;;
esac

echo "Compiling $mode executable..."

cd build || exit 1
if make -j "$target"; then
    cd .. || exit 1
    ./build/"$target"
else
    cd .. || exit 1
    exit 1
fi