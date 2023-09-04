# grep data
# Plot line and average
# create picture
#
import re
import numpy as np
import matplotlib.pyplot as plt

filename = 'r2m.txt'  # 文件名

# 读取文件内容
with open(filename, 'r') as file:
    data = file.readlines()

##############################################
# 使用正则表达式匹配包含 "real" 的行并提取时间
time_values = []
for line in data:
    if re.search(r"real", line):
        match = re.search(r"(\d+\.\d+)", line)
        if match:
            time_values.append(float(match.group(1)))

# 打印提取到的时间值
time_values_cubic = []
time_values_tcp = []
time_values_bbr = []
idx = 0
for value in time_values:
    if idx % 3 == 0:
        time_values_cubic.append(value)
    if idx % 3 == 1:
        time_values_tcp.append(value)
    if idx % 3 == 2:
        if int(value) > 10:
            time_values_bbr.append(1)
        else:
            time_values_bbr.append(value)
    idx += 1
    #print(value)

win = 5
x_values = range(win, len(time_values_cubic) + 1)
moving_average = np.convolve(time_values_cubic, np.ones(win) / win, mode='valid')
plt.figure()
plt.plot(x_values, time_values_cubic[win-1:], marker='o', label='Cubic')
plt.plot(x_values, moving_average, label='Average-'+str(win))
plt.title('Real Time Values of Cubic (s)')
plt.xlabel('Sample')
plt.ylabel('Time')
plt.savefig('./time_cubic.png')

x_values = range(win, len(time_values_tcp) + 1)
moving_average = np.convolve(time_values_tcp, np.ones(win) / win, mode='valid')
plt.figure()
plt.plot(x_values, time_values_tcp[win-1:], marker='o', label='TCP')
plt.plot(x_values, moving_average, label='Average-'+str(win))
plt.title('Real Time Values of TCP (s)')
plt.xlabel('Sample')
plt.ylabel('Time')
plt.savefig('./time_tcp.png')

x_values = range(win, len(time_values_bbr) + 1)
moving_average = np.convolve(time_values_bbr, np.ones(win) / win, mode='valid')
plt.figure()
plt.plot(x_values, time_values_bbr[win-1:], marker='o', label='BBR')
plt.plot(x_values, moving_average, label='Average-'+str(win))
plt.title('Real Time Values of BBR (s)')
plt.xlabel('Sample')
plt.ylabel('Time')
plt.savefig('./time_bbr.png')
#plt.show()

############################################
ping_values = []
idx = 0
for line in data:
    if re.search(r"time=", line):
        match = re.search(r"icmp_seq=(\d+) ttl=(\d+) time=(\d+)", line)
        if match:
            if idx % 3 == 1:
                ping_values.append(int(match.group(3)))
            idx += 1

#for value in ping_values:
#    print(value)
win = 5
x_values = range(win, len(ping_values) + 1)
moving_average = np.convolve(ping_values, np.ones(win) / win, mode='valid')
plt.figure()
plt.plot(x_values, ping_values[win-1:], marker='o', label='RTT')
plt.plot(x_values, moving_average, label='Average-'+str(win))
plt.title('Ping Values (ms)')
plt.xlabel('Sample')
plt.ylabel('Time')
plt.savefig('./ping.png')
