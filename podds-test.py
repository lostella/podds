#!/usr/bin/env python3

import sys
import subprocess

cmd_name = "./podds"
encoding = sys.stdout.encoding

test_cases = {
    "5 Ts 9d Js 8h 3c": \
        [0.315, 0.031, 0.367, 0.083, 0.014, 0.314, 0.000, 0.000, 0.000, 0.000],
    "4 Kd Jd Qs Tc Qd": \
        [0.406, 0.045, 0.291, 0.290, 0.051, 0.301, 0.039, 0.025, 0.001, 0.002],
}

for args in test_cases.keys():
    prob_ref = test_cases[args]
    cmd_list = [cmd_name]
    cmd_list.extend(args.split())
    res = subprocess.run(cmd_list, stdout=subprocess.PIPE)
    stdout_list = res.stdout.decode(encoding).split("\n")
    prob_list = list(map(lambda s: float(s.split(":")[1]), stdout_list[2:-1]))
    for k in range(len(prob_ref)):
        p1 = prob_ref[k]
        p2 = prob_list[k]
        err = abs(p1-p2)/(1+abs(p1))
        if err >= 1e-2:
            println("test {} failed".format(k))
            sys.exit(1)

println("all tests were succesful")
sys.exit(0)
