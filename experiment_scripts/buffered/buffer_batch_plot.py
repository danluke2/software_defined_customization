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

dns_data = [dns_data_base, dns_data_tap, dns_data_cust,
            dns_data_tap_buff, dns_data_cust_buff]


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
meanLabels = [str(np.round(s, 4)) for s in means]
# upperLabels2 = [str(np.round(s, 2)) for s in medians]


baseline = float(meanLabels[0])
tapOverhead = ((float(meanLabels[1]) - baseline)/baseline)
custOverhead = ((float(meanLabels[2]) - baseline)/baseline)
tapOverheadBuf = ((float(meanLabels[3]) - baseline)/baseline)
custOverheadBuf = ((float(meanLabels[4]) - baseline)/baseline)
percentLabels = ["", f'{tapOverhead:.2%}', f'{custOverhead:.2%}',
                 f'{tapOverheadBuf:.2%}', f'{custOverheadBuf:.2%}']


weights = ['bold', 'semibold']

for tick, label in zip(range(5), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.35, float(meanLabels[tick]), meanLabels[tick],
            horizontalalignment='center', weight=weights[k], color="green")
    ax.text(pos[tick]+0.35, float(meanLabels[tick])-0.08, percentLabels[tick],
            horizontalalignment='center', weight=weights[k], color="red")

plt.xticks(fontsize=14)
plt.xticks([1, 2, 3, 4, 5], ["Baseline", "L4.5 Tap", "L4.5 Tap+Cust",
           "L4.5 Tap", "L4.5 Tap+Cust"], rotation=45)
plt.ylabel('Seconds')
plt.title("DNS Batch Query/Response Time")

# spacing = 0.75
# fig.subplots_adjust(bottom=spacing)

# plt.show()
# plt.savefig('buffer_batch_overhead_combined.png')


# Testing a different king of plot
dns_data = [dns_data_base, dns_data_tap,  dns_data_tap_buff,
            dns_data_cust, dns_data_cust_buff]


fig, ax1 = plt.subplots(figsize=(10, 6))
fig.canvas.manager.set_window_title('A Boxplot Example')
fig.subplots_adjust(left=0.075, right=0.95, top=0.9, bottom=0.25)

bp = ax1.boxplot(dns_data, notch=0, sym='+', vert=1, whis=1.5, showmeans=True)
plt.setp(bp['boxes'], color='black')
plt.setp(bp['whiskers'], color='black')
plt.setp(bp['fliers'], color='red', marker='+')

# Add a horizontal grid to the plot, but make it very light in color
# so we can use it for reading data values but not be distracting
ax1.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',
               alpha=0.5)

ax1.set(
    axisbelow=True,  # Hide the grid behind plot objects
    title="DNS Batch Query/Response Time",
    xlabel='',
    ylabel='Seconds',
)

means = [item.get_ydata()[0] for item in bp['means']]

baseline = float(meanLabels[0])
tapOverhead = ((float(meanLabels[1]) - baseline)/baseline)
custOverhead = ((float(meanLabels[3]) - baseline)/baseline)
tapOverheadBuf = ((float(meanLabels[2]) - baseline)/baseline)
custOverheadBuf = ((float(meanLabels[4]) - baseline)/baseline)
percentLabels = ["", f'{tapOverhead:.2%}', f'{custOverhead:.2%}',
                 f'{tapOverheadBuf:.2%}', f'{custOverheadBuf:.2%}']

# Now fill the boxes with desired colors
box_colors = ['pink', 'darkkhaki', 'royalblue', 'darkkhaki', 'royalblue']
num_boxes = len(dns_data)
medians = np.empty(num_boxes)
for i in range(num_boxes):
    box = bp['boxes'][i]
    box_x = []
    box_y = []
    for j in range(5):
        box_x.append(box.get_xdata()[j])
        box_y.append(box.get_ydata()[j])
    box_coords = np.column_stack([box_x, box_y])
    # Alternate between Dark Khaki and Royal Blue
    ax1.add_patch(Polygon(box_coords, facecolor=box_colors[i]))
    # Now draw the median lines back over what we just filled in
    med = bp['medians'][i]
    median_x = []
    median_y = []
    for j in range(2):
        median_x.append(med.get_xdata()[j])
        median_y.append(med.get_ydata()[j])
        ax1.plot(median_x, median_y, 'k')
    medians[i] = median_y[0]
    # Finally, overplot the sample averages, with horizontal alignment
    # # in the center of each box
    # ax1.plot(np.average(med.get_xdata()), np.average(dns_data[i]),
    #          color='w', marker='*', markeredgecolor='k')

# Set the axes ranges and axes labels
ax1.set_xlim(0.5, num_boxes + 0.5)
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
ax1.set_ylim(bottom, top)
ax1.set_xticklabels(["Baseline", "L4.5 Tap\n(Strict Model)", "L4.5 Tap\n(Flexible Model)",
                     "L4.5 Tap+Cust\n(Strict Model)", "L4.5 Tap+Cust\n(Flexible Model)"],
                    rotation=0, fontsize=12)

# Due to the Y-axis scale being different across samples, it can be
# hard to compare differences in medians across the samples. Add upper
# X-axis tick labels with the sample medians to aid in comparison
# (just use two decimal places of precision)
pos = np.arange(num_boxes) + 1
upper_labels = [str(round(s, 2)) for s in means]
weights = ['bold', 'semibold']
for tick, label in zip(range(num_boxes), ax1.get_xticklabels()):
    k = tick % 2
    ax1.text(pos[tick], 0.95, upper_labels[tick],
             transform=ax1.get_xaxis_transform(),
             horizontalalignment='center', size='small',
             weight=weights[k], color="green")
    ax1.text(pos[tick], minimum - 0.1, percentLabels[tick],
             horizontalalignment='center', size='small',
             weight=weights[k], color="red")


# Finally, add a basic legend
# fig.text(0.80, 0.15, f'App Model',
#          backgroundcolor=box_colors[1], color='black', weight='roman',
#          size='large')
# fig.text(0.80, 0.1, 'Buffer Model',
#          backgroundcolor=box_colors[2],
#          color='white', weight='roman', size='large')
# fig.text(0.80, 0.015, '*', color='white', backgroundcolor='silver',
#          weight='roman', size='medium')
# fig.text(0.815, 0.013, ' Average Value', color='black', weight='roman',
#          size='x-small')

# plt.show()
plt.savefig('buffer_batch_overhead.png')
