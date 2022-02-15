# box plots of test scipts

# Import libraries
import matplotlib.pyplot as plt
import numpy as np


#run the bulk transfer shell script to collect the data, then generate the plots

#open each file, compare md5sum and fill in list of times
tcp_data_base = []
with open("logs/bulk_base.txt") as fp:
    md5compare = fp.readline()
    while True:
        md5download = fp.readline()
        if not md5download:
            break
        if md5compare != md5download:
            print(f"compare mismatch, {md5compare} != {md5download}")
        else:
            time = fp.readline()
            if not time:
                break
            tcp_data_base.append(int(time))


tcp_data_base = [x/1000 for x in tcp_data_base]
print(tcp_data_base)


tcp_data_tap = []
with open("logs/bulk_tap.txt") as fp:
    md5compare = fp.readline()
    while True:
        md5download = fp.readline()
        if not md5download:
            break
        if md5compare != md5download:
            print(f"compare mismatch, {md5compare} != {md5download}")
        else:
            time = fp.readline()
            if not time:
                break
            tcp_data_tap.append(int(time))


tcp_data_tap = [x/1000 for x in tcp_data_tap]
print(tcp_data_tap)


tcp_data_cust = []
with open("logs/bulk_cust.txt") as fp:
    md5compare = fp.readline()
    while True:
        md5download = fp.readline()
        if not md5download:
            break
        if md5compare != md5download:
            print(f"compare mismatch, {md5compare} != {md5download}")
        else:
            time = fp.readline()
            if not time:
                break
            tcp_data_cust.append(int(time))


tcp_data_cust = [x/1000 for x in tcp_data_cust]
print(tcp_data_cust)

tcp_data = [tcp_data_base, tcp_data_tap, tcp_data_cust]


plt.rc('axes', titlesize=16)     # fontsize of the axes title
plt.rc('axes', labelsize=14)
plt.rc('figure', titlesize=16)

fig, ax = plt.subplots()
bp = ax.boxplot(tcp_data, showmeans=True)

medians = [item.get_ydata()[0] for item in bp['medians']]
means = [item.get_ydata()[0] for item in bp['means']]
print(f'Medians: {medians}\n'
      f'Means:   {means}')


maximum = 0
minimum=1000000
for x in tcp_data:
    temp = max(x)
    if temp>maximum:
        maximum = temp
    temp = min(x)
    if temp<minimum:
        minimum = temp

top=maximum+0.5
bottom = minimum-0.5

ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',alpha=0.5)
pos = np.arange(3) + 1
meanLabels = [str(np.round(s, 2)) for s in means]
# upperLabels2 = [str(np.round(s, 2)) for s in medians]


baseline = float(meanLabels[0])
tapOverhead = ((float(meanLabels[1]) - baseline)/baseline)*100
custOverhead = ((float(meanLabels[2]) - baseline)/baseline)*100
percentLabels = ["", f'{tapOverhead:.2f}', f'{custOverhead:.2f}'] #math was done offline


weights = ['bold', 'semibold']

for tick, label in zip(range(3), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.35, float(meanLabels[tick]), meanLabels[tick],
             horizontalalignment='center', weight=weights[k], color="green")
    ax.text(pos[tick]+0.35, float(meanLabels[tick])-0.15, percentLabels[tick],
             horizontalalignment='center', weight=weights[k], color="red")

plt.xticks([1, 2, 3], ["Baseline", "L4.5 Tap", "L4.5 Tap+Cust"], rotation=0)
plt.ylabel('Seconds')
plt.title("Bulk File Transer Time")

# plt.show()
plt.savefig('bulk_overhead.png')
