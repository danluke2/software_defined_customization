# box plots of test scipts

# Import libraries
import matplotlib.pyplot as plt
import numpy as np


#open each file, compare md5sum and fill in list of times
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


server_data = [server_10_host, server_50_host, server_100_host, server_175_host, server_250_host]


fig, ax = plt.subplots()
bp = ax.boxplot(server_data, showmeans=True)
medians = [item.get_ydata()[0] for item in bp['medians']]
means = [item.get_ydata()[0] for item in bp['means']]
print(f'Medians: {medians}\n'
      f'Means:   {means}')


maximum = 0
for x in server_data:
    temp = max(x)
    if temp>maximum:
        maximum = temp

top=maximum+0.5
bottom = 0

ax.set_ylim(bottom, top)
ax.yaxis.grid(True, linestyle='-', which='major', color='lightgrey',alpha=0.5)
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
plt.xticks([1, 2, 3, 4, 5, 6], ["10", "50", "100", "175", "250", ""], rotation=0)
# plt.show()
plt.savefig('nco_deploy.png')
