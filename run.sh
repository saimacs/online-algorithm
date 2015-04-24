np=2
alpha=2
beta=2
numclusters=2
dimension=2
make clean;
make;
mpirun -np $np ./online $alpha $beta $numclusters $dimension;
