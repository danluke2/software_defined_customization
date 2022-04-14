# box plots of results used in the paper from manual experiments

# Import libraries
import matplotlib.pyplot as plt
import numpy as np



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

top=26.75
bottom = 24.50
ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',alpha=0.5)
pos = np.arange(3) + 1
upperLabels = [str(np.round(s, 2)) for s in means]
upperLabels2 = [str(np.round(s, 2)) for s in medians]
upperLabels3 = ["", "0.38%", "3%"] #math was done offline
weights = ['bold', 'semibold']
y_posit = [25.08, 25.15, 25.86]
for tick, label in zip(range(3), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.35, y_posit[tick]-0.05, upperLabels[tick],
             horizontalalignment='center', weight=weights[k], color="green")
    ax.text(pos[tick]+0.35, y_posit[tick]-0.15, upperLabels3[tick],
             horizontalalignment='center', weight=weights[k], color="red")

plt.xticks([1, 2, 3], ["Baseline", "L4.5 Tap", "L4.5 Tap+Cust"], rotation=0)
plt.ylabel('Seconds')
plt.title("Bulk File Transfer Time")

# plt.show()
plt.savefig('tcp_overhead.png')



udp_data_base = [7672.926, 7683.755, 7704.190, 7623.504, 7822.803, 7652.786, 7706.702, 7781.706, 7720.194, 7778.815, 7702.554, 7716.742, 7694.213, 7705.852, 7587.036]
udp_data_tap = [7785.877, 7830.522, 7682.462, 7756.673, 7821.008, 7787.260, 7768.021, 7698.633, 7790.228, 7791.322, 7987.967, 7784.721, 7840.865, 7753.418, 7912.446]
udp_data_cust = [7755.357, 8005.567, 7755.329, 7788.964, 7832.619, 7743.185, 7934.737, 7915.508, 7792.570, 7869.995, 7763.449, 7845.715, 7931.759, 7762.519, 7773.534]

udp_data_base = [x/1000 for x in udp_data_base]
udp_data_tap = [x/1000 for x in udp_data_tap]
udp_data_cust = [x/1000 for x in udp_data_cust]
udp_data = [udp_data_base, udp_data_tap, udp_data_cust]


fig, ax = plt.subplots()
bp = ax.boxplot(udp_data, showmeans=True)
medians = [item.get_ydata()[0] for item in bp['medians']]
means = [item.get_ydata()[0] for item in bp['means']]
print(f'Medians: {medians}\n'
      f'Means:   {means}')

y_posit = [7.703, 7.799, 7.831]
top=8.010
bottom = 7.5
ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',alpha=0.5)
pos = np.arange(3) + 1
upperLabels = [str(np.round(s, 2)) for s in means]
upperLabels2 = [str(np.round(s, 2)) for s in medians]
upperLabels3 = ["", "1.2%", "1.6%"] #math was done offline
weights = ['bold', 'bold']
for tick, label in zip(range(3), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.3, y_posit[tick]-.01, upperLabels[tick],
             horizontalalignment='center', weight=weights[k], color="green")
    ax.text(pos[tick]+0.3, y_posit[tick]-.03, upperLabels3[tick],
             horizontalalignment='center', weight=weights[k], color="red")
# for key in bp:
#     print(f'{key}: {[item.get_ydata() for item in bp[key]]}\n')
plt.ylabel('Seconds')

# displaying the title
plt.title("DNS Batch Query/Response Time")
plt.xticks([1, 2, 3], ["Baseline", "L4.5 Tap", "L4.5 Tap+Cust"], rotation=0)
# plt.show()
plt.savefig('udp_overhead_sec.png')





server_10_host = [0.342, 0.359, 0.487, 0.351, 0.361, 0.322, 0.326, 0.377, 0.354, 0.440, 0.324, 0.385, 0.350, 0.330, 0.353]
server_50_host = [1.979, 1.754, 1.931, 1.918, 2.054, 2.117, 2.046, 1.915, 1.935, 2.388, 2.052, 2.129, 2.104, 2.310, 1.788]
server_100_host = [4.681, 4.324, 4.892, 5.380, 5.509, 5.229, 4.455, 4.357, 4.867, 4.331, 4.634, 5.517, 5.027, 4.808, 4.517]
server_175_host = [10.036, 9.797, 9.951, 8.764, 9.240, 13.249, 10.379, 10.105, 9.941, 10.372, 13.647, 9.999, 11.060, 12.663, 10.041]
server_250_host = [13.915, 13.760, 14.562, 13.029, 12.490, 14.523, 16.452, 12.001, 14.601, 13.890, 13.888, 15.117, 15.984, 16.040, 16.363]
server_data = [server_10_host, server_50_host, server_100_host, server_175_host, server_250_host]


fig, ax = plt.subplots()
bp = ax.boxplot(server_data, showmeans=True)
medians = [item.get_ydata()[0] for item in bp['medians']]
means = [item.get_ydata()[0] for item in bp['means']]
print(f'Medians: {medians}\n'
      f'Means:   {means}')

y_posit = [0.75, 2.2, 5, 10.5, 14.5]
top=17
bottom = 0
ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',alpha=0.5)
pos = np.arange(5) + 1
upperLabels = [str(np.round(s, 2)) for s in means]
upperLabels2 = [str(np.round(s, 2)) for s in medians]
weights = ['bold', 'bold']
for tick, label in zip(range(5), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.55, y_posit[tick]-0.25, upperLabels[tick],
             horizontalalignment='center', weight=weights[k], color="green")
    # ax.text(pos[tick]+0.47, y_posit[tick] - 0.5, upperLabels2[tick],
    #          horizontalalignment='center', size='x-small', weight=weights[k], color="orange")
# for key in bp:
#     print(f'{key}: {[item.get_ydata() for item in bp[key]]}\n')
plt.ylabel('Seconds')
plt.xlabel('Devices')

# displaying the title
plt.title("NCO Module Deployment Time")
plt.xticks([1, 2, 3, 4, 5, 6], ["10", "50", "100", "175", "250", ""], rotation=0)
# plt.show()
plt.savefig('nco_deploy.png')
