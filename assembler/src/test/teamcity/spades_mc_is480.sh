
#!/bin/bash

############################################################################
# Copyright (c) 2011-2012 Saint-Petersburg Academic University
# All Rights Reserved
# See file LICENSE for details.
############################################################################

set -e
pushd ../../../
rm -rf spades_output/ECOLI_IS480_QUAKE
./spades.py --config-file src/test/teamcity/spades_config_mc_is480.info
rm -rf ~/quast-1.1/ECOLI_IS480_QUAKE/
python ~/quast-1.1/quast.py -R data/input/E.coli/MG1655-K12.fasta.gz -G data/input/E.coli/genes/genes.gff -O data/input/E.coli/genes/operons.gff -o ~/quast-1.1/ECOLI_IS480_QUAKE/  spades_output/ECOLI_IS480_QUAKE/contigs.fasta
python src/test/teamcity/assess.py ~/quast-1.1/ECOLI_IS480_QUAKE/transposed_report.tsv 133000 4
popd