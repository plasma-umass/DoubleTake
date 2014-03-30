#!/usr/bin/python

import os
import sys
import subprocess
import re

all_benchmarks = os.listdir('tests')
all_benchmarks.remove('Makefile')
all_benchmarks.remove('defines.mk')
all_benchmarks.remove('readme')
all_benchmarks.sort()

#all_configs = ['pthread', 'defaults']
#all_configs = ['pthread', 'defaults']
all_configs = ['doubletake']
runs = 1

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
				
				start_time = os.times()[4]
				#os.system("sudo sh -c \"sync; echo 3 > /proc/sys/vm/drop_caches\";");	
				p = subprocess.Popen(['make', 'eval-'+config, 'NCORES='+str(cores)], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        
				p.wait()
				
				time = os.times()[4] - start_time
				data[benchmark][config].append(time)
	
				os.chdir('../..')

except:
	print 'Aborted!'

print 'benchmark',
for config in configs:
	print '\t'+config,
print

output=open("./result.txt", "w")
for benchmark in benchmarks:
	print benchmark,
	output.write("%s\t"%benchmark);
	for config in configs:
		if benchmark in data and config in data[benchmark] and len(data[benchmark][config]) == runs:
			if len(data[benchmark][config]) >= 4:
				mean = (sum(data[benchmark][config])-max(data[benchmark][config])-min(data[benchmark][config]))/(runs-2)
			else:
				mean = sum(data[benchmark][config])/runs
			print '\t'+str(mean),
			output.write("%s\t"%mean);
		else:
			print '\tNOT RUN',
	print
	output.write("\n");

output.close();
