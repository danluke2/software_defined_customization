# box plots of test scipts

# Import libraries
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Polygon


# run the bulk transfer shell script to collect the data, then generate the plots

# open each file, compare md5sum and fill in list of times
dns_data_base = []
with open("../logs/batch_base.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        dns_data_base.append(int(time))

dns_data_base = [x/1000000 for x in dns_data_base]
print(dns_data_base)


dns_data_tap_buff = []
with open("../logs/buffer_batch_tap.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        dns_data_tap_buff.append(int(time))


dns_data_tap_buff = [x/1000000 for x in dns_data_tap_buff]
print(dns_data_tap_buff)


dns_data_cust_buff = []
with open("../logs/buffer_batch_cust.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        dns_data_cust_buff.append(int(time))


dns_data_cust_buff = [x/1000000 for x in dns_data_cust_buff]
print(dns_data_cust_buff)

dns_data = [dns_data_base, dns_data_tap_buff, dns_data_cust_buff]


plt.rc('axes', titlesize=16)     # fontsize of the axes title
plt.rc('axes', labelsize=14)
plt.rc('figure', titlesize=16)

fig, ax = plt.subplots()
bp = ax.boxplot(dns_data, showmeans=True)

medians = [item.get_ydata()[0] for item in bp['medians']]
means = [item.get_ydata()[0] for item in bp['means']]
print(f'Medians: {medians}\n'
      f'Means:   {means}')


maximum = 0
minimum = 1000000
for x in dns_data:
    temp = max(x)
    if temp > maximum:
        maximum = temp
    temp = min(x)
    if temp < minimum:
        minimum = temp

top = maximum+0.2
bottom = minimum-0.2

ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey', alpha=0.5)
pos = np.arange(5) + 1
meanLabels = [str(np.round(s, 2)) for s in means]
# upperLabels2 = [str(np.round(s, 2)) for s in medians]


baseline = float(meanLabels[0])
tapOverheadBuf = ((float(meanLabels[1]) - baseline)/baseline)
custOverheadBuf = ((float(meanLabels[2]) - baseline)/baseline)
percentLabels = ["", f'{tapOverheadBuf:.2%}', f'{custOverheadBuf:.2%}']


weights = ['bold', 'semibold']

for tick, label in zip(range(5), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.35, float(meanLabels[tick]), meanLabels[tick],
            horizontalalignment='center', weight=weights[k], color="green")
    ax.text(pos[tick]+0.35, float(meanLabels[tick])-0.08, percentLabels[tick],
            horizontalalignment='center', weight=weights[k], color="red")

plt.xticks(fontsize=14)
plt.xticks([1, 2, 3], ["Baseline", "L4.5 Tap", "L4.5 Tap+Cust"])
plt.ylabel('Seconds')
plt.title("DNS Batch Query/Response Time")


# plt.show()
plt.savefig('buffer_batch_overhead.png')
