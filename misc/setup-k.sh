#!/usr/bin/env bash

set -x

./bootstrap

module_path="/u/shared/fun3d/fun3d_users/modules"
zoltan_path="${module_path}/Zoltan/3.82-openmpi-1.10.2-intel_2017.2.174"
zoltan_path="/u/mpark/local/pkgs/Zoltan_v3.82/gcc_6.2.0-mpich_3.2"
egads_path="${module_path}/ESP/112/EngSketchPad"

mkdir -p strict
( cd strict && \
    ../configure \
    --prefix=`pwd` \
    CFLAGS='-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    FC=gfortran \
    ) \
    || exit

mkdir -p egads
( cd egads && \
    ../configure \
    --prefix=`pwd` \
    --with-EGADS=${egads_path} \
    CFLAGS='-g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    FC=gfortran \
    ) \
    || exit

mkdir -p zoltan
( cd zoltan && \
    ../configure \
    --prefix=`pwd` \
    --with-zoltan=${zoltan_path} \
    --with-EGADS=${egads_path} \
    --enable-lite \
    CC=mpicc \
    FC=mpif90 \
    CFLAGS='-DHAVE_MPI -g -O2 -pedantic-errors -Wall -Wextra -Werror -Wunused -Wuninitialized' \
    ) \
    || exit
