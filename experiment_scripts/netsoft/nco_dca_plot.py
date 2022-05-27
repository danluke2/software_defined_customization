# box plots of test scipts

# Import libraries
import matplotlib.pyplot as plt
import numpy as np


# open each file, compare md5sum and fill in list of times
server_10_host = []
with open("../logs/nco_results_10.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        server_10_host.append(int(time))


server_10_host = [x/1000 for x in server_10_host]
print(server_10_host)


server_50_host = []
with open("../logs/nco_results_50.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        server_50_host.append(int(time))


server_50_host = [x/1000 for x in server_50_host]
print(server_50_host)


server_100_host = []
with open("../logs/nco_results_100.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        server_100_host.append(int(time))


server_100_host = [x/1000 for x in server_100_host]
print(server_100_host)


server_175_host = []
with open("../logs/nco_results_175.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        server_175_host.append(int(time))


server_175_host = [x/1000 for x in server_175_host]
print(server_175_host)


server_250_host = []
with open("../logs/nco_results_250.txt") as fp:
    while True:
        time = fp.readline()
        if not time:
            break
        server_250_host.append(int(time))


server_250_host = [x/1000 for x in server_250_host]
print(server_250_host)


server_data = [server_10_host, server_50_host,
               server_100_host, server_175_host, server_250_host]


fig, ax = plt.subplots()
bp = ax.boxplot(server_data, showmeans=True)
medians = [item.get_ydata()[0] for item in bp['medians']]
means = [item.get_ydata()[0] for item in bp['means']]
print(f'Medians: {medians}\n'
      f'Means:   {means}')


maximum = 0
for x in server_data:
    temp = max(x)
    if temp > maximum:
        maximum = temp

top = maximum+0.5
bottom = 0

ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey', alpha=0.5)
pos = np.arange(5) + 1
meanLabels = [str(np.round(s, 2)) for s in means]
weights = ['bold', 'bold']
for tick, label in zip(range(5), ax.get_xticklabels()):
    k = tick % 2
    ax.text(pos[tick]+0.55, float(meanLabels[tick]), meanLabels[tick],
            horizontalalignment='center', weight=weights[k], color="green")

plt.ylabel('Seconds')
plt.xlabel('Devices')

plt.title("NCO Module Deployment Time")
plt.xticks([1, 2, 3, 4, 5, 6], ["10", "50",
           "100", "175", "250", ""], rotation=0)
# plt.show()
plt.savefig('nco_deploy.png')


# Results from 5/26/22
# [0.262, 0.321, 0.284, 0.291, 0.268, 0.287, 0.296, 0.259, 0.275, 0.282, 0.274, 0.278, 0.267, 0.276, 0.307]
# [1.356, 1.458, 1.462, 1.489, 1.589, 1.493, 1.414, 1.422, 1.439, 1.479, 1.476, 1.425, 1.445, 1.485, 1.456]
# [3.158, 3.13, 3.182, 3.083, 3.165, 3.264, 3.241, 3.018, 3.132, 3.192, 3.635, 3.346, 3.137, 3.166, 3.118]
# [5.592, 6.404, 5.995, 6.065, 5.973, 5.956, 6.028, 5.747, 6.271, 6.081, 5.874, 6.258, 5.815, 5.504, 5.912]
# [8.228, 8.019, 26.52, 7.935, 12.404, 8.159, 20.502, 8.279, 8.532, 8.697, 8.895, 7.84, 9.1, 9.117, 8.019]
# Medians: [0.278, 1.458, 3.165, 5.973, 8.532]
# Means:   [0.2818, 1.4591999999999998, 3.1978000000000004, 5.965000000000001, 10.683066666666665]
