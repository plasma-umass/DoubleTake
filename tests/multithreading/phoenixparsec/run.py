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
all_benchmarks = ['blackscholes', 'bodytrack', 'dedup', 'ferret', 'fluidanimate', 'histogram', 'kmeans','linear_regression', 'reverse_index', 'matrix_multiply', 'pca', 'streamcluster', 'string_match', 'swaptions', 'word_count', 'x264'] 
#all_configs = ['pthread', 'dmp_o', 'dmp_b', 'dthread']
all_configs = ['pthread', 'doubletake']
#all_configs = ['defaults']
runs = 4

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
				p.wait()
				os.chdir('../..')

except:
	print 'Aborted!'
	
for benchmark in benchmarks:
	print benchmark;
