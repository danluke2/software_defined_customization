# box plots of test scipts

# Import libraries
import matplotlib.pyplot as plt
import numpy as np


#run the bulk transfer shell script to collect the data, then generate the plots

# actually, need higher level script to setup each phase: base, layer, cust
# client connect to server over ssh, launch web server then on client run experiment, save data to file
# client connect to server over ssh, install L4.5, launch server, client install L4.5, run experiment, save data to file
# client connect to server over ssh, install module, launch server, client install module, run experiment, save data to file
# import data from file into this program to plot data

tcp_data_base = [25.300, 24.651, 25.091, 25.675, 25.106, 24.588, 24.788, 25.077, 25.532, 25.209, 24.753, 25.160, 25.043, 24.802, 24.839]
tcp_data_tap = [24.922, 24.908, 24.755, 25.088, 25.645, 24.939, 24.961, 25.109, 25.022, 25.152, 25.345, 25.264, 25.326, 24.888, 25.723]
tcp_data_cust_1k = [25.791, 25.843, 25.412, 25.587, 25.622, 26.594, 26.112, 25.659, 25.464, 25.396, 26.116, 25.777, 25.964, 26.328, 25.646]
tcp_data = [tcp_data_base, tcp_data_tap, tcp_data_cust_1k]

# plt.rcParams.update({'font.size': 12})
plt.rc('axes', titlesize=16)     # fontsize of the axes title
plt.rc('axes', labelsize=14)
plt.rc('figure', titlesize=16)

fig, ax = plt.subplots()
bp = ax.boxplot(tcp_data, showmeans=True)
# for key in bp:
#     print(f'{key}: {[item.get_ydata() for item in bp[key]]}\n')
medians = [item.get_ydata()[0] for item in bp['medians']]
means = [item.get_ydata()[0] for item in bp['means']]
print(f'Medians: {medians}\n'
      f'Means:   {means}')

#auto set these values
top=26.75
bottom = 24.50
ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',alpha=0.5)
pos = np.arange(3) + 1
upperLabels = [str(np.round(s, 2)) for s in means]
upperLabels2 = [str(np.round(s, 2)) for s in medians]
#auto set these values
upperLabels3 = ["", "0.38%", "3%"] #math was done offline
weights = ['bold', 'semibold']
#auto set these values
y_posit = [25.08, 25.15, 25.86]
for tick, label in zip(range(3), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.35, y_posit[tick]-0.05, upperLabels[tick],
             horizontalalignment='center', weight=weights[k], color="green")
    ax.text(pos[tick]+0.35, y_posit[tick]-0.15, upperLabels3[tick],
             horizontalalignment='center', weight=weights[k], color="red")

plt.xticks([1, 2, 3], ["Baseline", "L4.5 Tap", "L4.5 Tap+Cust"], rotation=0)
plt.ylabel('Seconds')
plt.title("Bulk File Transer Time")

# plt.show()
plt.savefig('bulk_overhead.png')
