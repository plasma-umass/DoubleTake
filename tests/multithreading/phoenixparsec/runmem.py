#!/usr/bin/python

import os
import sys
import subprocess
import re

#all_benchmarks = os.listdir('tests')
#all_benchmarks.remove('Makefile')
#all_benchmarks.remove('defines.mk')
#all_benchmarks.sort()

#all_benchmarks = os.listdir('tests')
#all_benchmarks.remove('Makefile')
#all_benchmarks.remove('defines.mk')
#all_benchmarks = ['blackscholes', 'bodytrack', 'dedup', 'ferret', 'fluidanimate', 'histogram', 'kmeans','linear_regression', 'reverse_index', 'matrix_multiply', 'pca', 'streamcluster', 'string_match', 'swaptions', 'word_count', 'x264'] 
all_benchmarks = ['aget', 'pfscan', 'pbzip2']
#all_configs = ['pthread', 'dmp_o', 'dmp_b', 'dthread']
all_configs = ['pthread', 'defaults']
#all_configs = ['defaults']
runs = 1

cores = '8'

benchmarks = all_benchmarks
configs = all_configs

data = {}
try:
	for benchmark in benchmarks:
		data[benchmark] = {}
		for config in configs:
			data[benchmark][config] = []
	
			for n in range(0, runs):
				print 'Running '+benchmark+'.'+config
				os.chdir('tests/'+benchmark)
				p = subprocess.Popen(['make', 'eval-'+config, 'NCORES='+str(cores)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
				p2 = subprocess.call('/nfs/cm/scratch1/tonyliu/grace/branches/falsesharing/project/tools/savemem-simple.sh $benchmark')
				p.wait()
				os.chdir('../..')

except:
	print 'Aborted!'
	
for benchmark in benchmarks:
	print benchmark;
