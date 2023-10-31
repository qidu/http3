# cat result-kcp.log | grep -E "100% | http://" | grep -v "9550K" > result-kcp3.log

import matplotlib.pyplot as plt

idx = 0
url = ""
tm = ""
res = {}

with open('result-kcp-suf1.log') as f:
    readurl = False
    readtime = False
    for line in f:
        part = line.split(' ')
        if len(part) == 4:
            url = part[3]
            url = url.replace("\n", "").replace("\r", "").replace(" ", "").replace("http:", "").replace("/", "").replace("kcptunclient", "")
            readurl = True
        part = line.split('=')
        if len(part) == 2:
            tm = part[1]
            tm = tm.replace("\n", "").replace("\r", "").replace(" ", "").replace("s", "")
            readtime = True
        if idx % 2 == 1:
            if not (readurl and readtime): # skip 2 lines
                url = ""
                tm = 0
                idx += 1
                continue
            #print(url, tm)
            if url not in res:
                res[url] = []
            res[url].append(float(tm))
        idx += 1
print(res)
for key, values in res.items():
    plt.plot(values, label=key)

plt.legend()
plt.title('TCP vs KCP @Suf')
plt.xlabel('Tests')
plt.ylabel('Time(s)')
#plt.show()
plt.savefig('./testkcp_suf.png')
