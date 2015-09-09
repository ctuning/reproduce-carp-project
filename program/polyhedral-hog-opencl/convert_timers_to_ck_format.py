#
# Converting raw HOG timing to CK universal
# autotuning and machine learning format.
#
# Collective Knowledge (CK)
#
# See CK LICENSE.txt for licensing details.
# See CK COPYRIGHT.txt for copyright details.
#
# Developer: Grigori Fursin
#

import json

d={}

print ('  (converting fine-grain timers from HOG to CK format ...)')

# Preload tmp-ck-timer.json from OpenME if there
exists=True
try:
  f=open('tmp-ck-timer.json', 'r')
except Exception as e:
  exists=False
  pass

if exists:
   try:
     s=f.read()
     d=json.loads(s)
   except Exception as e:
     exists=False
     pass

   if exists:
      f.close()

print ('  (parsing output from stdout.log ...)')

# Load kernel info from HOG
f=open('stdout.log','r')
s=f.read()
f.close()

desc={} # -> should move to meta desc of this program

l=s.split('\n')
ll=len(l)


cpu=0.0
gpu_only=0.0
gpu_with_mem=0.0

n=0
for x in l:
    i1=x.find('calc_hog execution time:')
    i2=x.find(' ms')
    if i1>=0 and i2>=0:
       i1x=x.find(':',i1)
       if i1x>=0:
          t=x[i1x+1:i2].strip()
          t=float(t)/1000
          gpu_only+=t
          n+=1 # how many repetitions detected

    if x.startswith('[RealEyes] Accumulate CPU time           : '):
       cpu=float(x[43:].strip())/1000

    if x.startswith('[RealEyes] Accumulate GPU time (inc copy): '):
       gpu_with_mem=float(x[43:].strip())/1000

if n!=0:
   gpu_only=gpu_only/n # divide by number of repetitions

d['dim_cpu']=cpu
d['dim_gpu_only']=gpu_only
d['dim_gpu_with_mem']=gpu_with_mem


if gpu_only!=0.0:
   d['derived_cpu_time_div_by_gpu_only_time']=cpu/gpu_only
   d['derived_gpu_with_mem_time_div_by_gpu_only_time']=gpu_with_mem/gpu_only
if gpu_with_mem!=0.0:
   d['derived_cpu_time_div_by_gpu_with_mem_time']=cpu/gpu_with_mem


if ((gpu_with_mem!=0.0) and ((cpu/gpu_with_mem)>1.07)):
   d['derived_gpu_with_mem_is_much_better_cpu']=True
else:
   d['derived_gpu_with_mem_is_much_better_cpu']=False

if ((gpu_only!=0.0) and ((cpu/gpu_only)>1.07)):
   d['derived_gpu_only_is_much_better_cpu']=True
else:
   d['derived_gpu_only_is_much_better_cpu']=False


# Write CK json
f=open('tmp-ck-timer.json','wt')
f.write(json.dumps(d, indent=2, sort_keys=True)+'\n')
f.close()
