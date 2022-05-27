# box plots of test scipts

# Import libraries
import matplotlib.pyplot as plt
import numpy as np


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


dns_data_tap = []
with open("../logs/batch_tap.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        dns_data_tap.append(int(time))


dns_data_tap = [x/1000000 for x in dns_data_tap]
print(dns_data_tap)


dns_data_cust = []
with open("../logs/batch_cust.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        dns_data_cust.append(int(time))


dns_data_cust = [x/1000000 for x in dns_data_cust]
print(dns_data_cust)

dns_data = [dns_data_base, dns_data_tap, dns_data_cust]


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
pos = np.arange(3) + 1
meanLabels = [str(np.round(s, 2)) for s in means]
# upperLabels2 = [str(np.round(s, 2)) for s in medians]


baseline = float(meanLabels[0])
tapOverhead = ((float(meanLabels[1]) - baseline)/baseline)
custOverhead = ((float(meanLabels[2]) - baseline)/baseline)
percentLabels = ["", f'{tapOverhead:.2%}', f'{custOverhead:.2%}']


weights = ['bold', 'semibold']

for tick, label in zip(range(3), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.35, float(meanLabels[tick]), meanLabels[tick],
            horizontalalignment='center', weight=weights[k], color="green")
    ax.text(pos[tick]+0.35, float(meanLabels[tick])-0.15, percentLabels[tick],
            horizontalalignment='center', weight=weights[k], color="red")

plt.xticks(fontsize=14)
plt.xticks([1, 2, 3], ["Baseline", "L4.5 Tap", "L4.5 Tap+Cust"], rotation=0)
plt.ylabel('Seconds')
plt.title("DNS Batch Query/Response Time")

# plt.show()
plt.savefig('batch_overhead.png')
