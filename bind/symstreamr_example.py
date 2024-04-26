#!/usr/bin/env python3
'''demonstrate symbol streaming generator'''
import argparse, sys
sys.path.extend(['.','..'])
import liquid as dsp, numpy as np, matplotlib.pyplot as plt

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('-nodisplay', action='store_true', help='disable display')
args = p.parse_args()

num_samples = 4e6

# create generator
gen = dsp.symstreamr(ms="bpsk", bw=0.25)

# create power spectral density estimator
psd = dsp.spgram(nfft=600,wlen=400,delay=10)

buffer = []

print(gen,gen.delay)
while psd.num_samples_total < num_samples:
    buf = gen.generate(2400)
    psd.execute(buf)
    buffer.extend(buf.tolist())


# get spectrum plot and display
Sxx,f = psd.get_psd()

fix,ax = plt.subplots(2,1,figsize=(8,8))
ax[0].plot(f,Sxx)
ax[0].set_xlabel('Normalized Frequency [f/Fs]')
ax[0].set_ylabel('Time [samples]')
ax[0].grid(True, zorder=5)
ax[0].set(xlim=(-0.5,0.5))
ax[0].set_xticks(np.linspace(-0.5,0.5,11))
ax[1].plot(np.abs(buffer))
if not args.nodisplay:
    plt.show()

