import numpy as np
import pandas as pd
import os,sys
import seaborn
import subprocess
import matplotlib.pyplot as plt

seaborn.set(style='ticks')
# fig, ax = plt.subplots(nrows=2, ncols=1)
bash_command = './ass'
N = [10,50,100]
times = 10
_Algo = ['FCFS','NPSJF','PSJF','RR','HRN']
data = []
row_no = 0
df = pd.DataFrame(columns=['N','Algo','Itr','ATN'] )
for n in N:
    entry = []
    for t in range(times):
        out = subprocess.Popen([bash_command,str(n)],
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
        stout, stderr = out.communicate()
        stout = stout.decode("utf-8")
        stout = [float(x) for x in stout.split('\n')[:-1]]
        for i,alg in enumerate(stout):
            df.loc[row_no] = {'N': n, 'Algo': _Algo[i], 'Itr': t, 'ATN': alg}
            row_no+=1
# print(df)
df2 = df.groupby(['N', 'Algo'])['ATN'].mean().reset_index()
print(df2)
fg1 = seaborn.FacetGrid(data=df, hue='Algo', hue_order=_Algo,aspect=1.61)
fg1.map(plt.scatter, 'N', 'ATN').add_legend()
fg2 = seaborn.FacetGrid(data=df2, hue='Algo', hue_order=_Algo, aspect=1.61)
fg2.map(plt.plot, 'N', 'ATN').add_legend()
plt.show()
