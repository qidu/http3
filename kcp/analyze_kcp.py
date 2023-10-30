# cat result-kcp.log | grep -E "100% | http://" | grep -v "9550K" > result-kcp3.log

import matplotlib.pyplot as plt

idx = 0
url = ""
tm = ""
res = {}

with open('result-kcp3.log') as f:
    for line in f:
        part = line.split(' ')

        if len(part) == 4:
            url = part[3]
            url = url.replace("\n", "").replace("\r", "").replace(" ", "").replace("http:", "").replace("/", "").replace("kcptunclient", "")
        part = line.split('=')
        if len(part) == 2:
            tm = part[1]
            tm = tm.replace("\n", "").replace("\r", "").replace(" ", "").replace("s", "")
        if idx % 2 == 1:
            #print(url, tm)
            if url not in res:
                res[url] = []
            res[url].append(float(tm))
        idx += 1
print(res)
for key, values in res.items():
    plt.plot(values, label=key)

plt.legend()
plt.title('TCP vs KCP')
plt.xlabel('Tests')
plt.ylabel('Time(s)')

#plt.show()
plt.savefig('./testkcp.png')
