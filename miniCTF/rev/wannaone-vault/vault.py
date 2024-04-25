import sys

num = [0x577020,0x302071,0x797022,0x612073,0x357024,0x712075,0x597026,0x372077,0x787028,0x3a2079,0x78702a,0x6a207b,0x78702c,0x3c207d,0x3e702e,0x61207f,0x4f7030,0x672061,0x737032,0x662063,0x787034,0x612065,0x497036,0x702067,0x777038,0x762069,0x7e703a,0x44206b,0x76703c,0x2d206d,0x7c703e,0x62206f]

flag = []
j = 0
for key in num:
    if j % 2 == 0:
        for x in range(256):
            if (((((x ^ j) << 0x10) ^ 0x7020) ^ j) == key):
                flag.append(x)
                break
    else:
        for y in range(256):
            if (((((y ^ j) << 0x10) ^ 0x2070) ^ j) == key):
                flag.append(y)
                break
    j += 1

print(''.join(chr(i) for i in flag))
