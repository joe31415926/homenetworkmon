#!/usr/bin/python3
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot
import matplotlib.dates
import datetime
import sys

A = []
B = []
for line in sys.stdin:
    cols = line.split()
    if len(cols) >= 3:
        A.append(matplotlib.dates.date2num(datetime.datetime.fromtimestamp(float(cols[0]))))
        B.append(float(cols[2]))

fig = matplotlib.pyplot.figure(figsize=[5,3])
ax = fig.add_subplot(111)
ax.plot_date(A, B, '.')
ax.set_ylabel("milliseconds")
ax.xaxis.set_major_formatter(matplotlib.dates.DateFormatter('%H:%M'))
ax.invert_xaxis()
matplotlib.pyplot.savefig(sys.stdout.buffer, format='rgba', dpi=100)
