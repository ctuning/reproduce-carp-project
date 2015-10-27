#
# Converting raw penci-benchmark timing to CK universal 
# autotuning and machine learning format
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

print ('Converting fine-grain timers from penci-benchmark to CK format ...')

# Load kernel info from slambench
f=open('stdout.log','r')
s=f.read()
f.close()

d={}

dcompute=0.0

l=s.split('\n')
ll=len(l)
if ll>0:
   for q in l:
       qn=q.strip()
       if qn.startswith('compute'):
          i1=qn.find(':')
          if i1>0:
             i2=qn.find('ms')
             if i2>0:
                qq=qn[i1+1:i2].strip()
                dcompute+=float(qq)

if dcompute>0:
   dcompute/=1e6
   d['execution_time_kernel_0']=dcompute
   d['execution_time']=dcompute

# Write CK json
f=open('tmp-ck-timer.json','wt')
f.write(json.dumps(d, indent=2, sort_keys=True)+'\n')
f.close()
