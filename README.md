# WATT Simulation Framework

Here you can find our Simulation Framework used in our WATT paper.
You can find an overview of all artifacts used in our [paper here](https://github.com/leanstore/leanstore/blob/WATT/README.md).

## Compiling

``` bash
mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo .. && make -j
```

## Running

``` bash
./evalAccessTable tracefile outdir
```

## Adding Strategies

New strategies can be added in `/algos/` and have to be linked in `/algos/Generators.cpp` and `/algos/Generators.hpp`.

## Adding Runconfigurations

Configurations can be added in `/evalAccessTable/evalAccessTable.cpp`.

## Citing

If you use this in your work, please cite the [WATT paper]((https://github.com/leanstore/leanstore/blob/WATT/README.md)).

```bibtex
@article{WATT23,
author = {V\"{o}hringer, Demian and Leis, Viktor},
title = {Write-Aware Timestamp Tracking: Effective and Efficient Page Replacement for Modern Hardware},
year = {2023},
issue_date = {July 2023},
publisher = {VLDB Endowment},
volume = {16},
number = {11},
issn = {2150-8097},
url = {https://doi.org/10.14778/3611479.3611529},
doi = {10.14778/3611479.3611529},
journal = {Proc. VLDB Endow.},
month = {aug},
pages = {3323–3334},
numpages = {12}
}
```

## Contact

If you are having any problems or suggestions, feel free to [open an issue](https://github.com/itodnerd/WATT-simulate/issues/new/choose).
