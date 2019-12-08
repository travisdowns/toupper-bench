#!/usr/bin/env python3

import subprocess
import argparse
import re
import sys
import json
import statistics

aggr_fns = {
    "median" : lambda x : [statistics.median(x)],
    "min" : lambda x : [min(x)],
    "max" : lambda x : [max(x)],
    "all" : lambda x: x
}

# for arguments that should be comma-separate lists, we use splitlsit as the type
splitlist = lambda x: x.split(',')

p = argparse.ArgumentParser(usage='drive the benchmark program')
p.add_argument('--aggr', help='The aggregation function to use', default="median", choices=aggr_fns)
p.add_argument('--benches', type=lambda x: x.split(','), help='list of benchmarks to include in the output')
p.add_argument('--base-env', type=json.loads, help='The base (unvarying) environment as a json string')
p.add_argument('--xvar', required=True, help='The variable to vary (generally the x axis), in the form VAR=START-INCR-END')
p.add_argument('--yvars', help='The metrics to use as the data points', type=splitlist, default=['Cycles'])

args = p.parse_args()

aggr = aggr_fns[args.aggr]

xvar_re = re.compile(r'(\w+)=(\d+)-(\d+)-(\d+)')
re_match = xvar_re.match(args.xvar)
if not re_match:
    sys.exit('xvar should look like VAR=START-INCR-END with START, INCR and END all integers')

if (len(args.yvars) == 0): sys.exit('must have at least one yval')

xvar = re_match.group(1)
xstart = int(re_match.group(2))
xincr  = int(re_match.group(3))
xend   = int(re_match.group(4))

cmd = ['./bench', ','.join(args.benches)] if args.benches else ['./bench']

print("{} from {} to {}, step {}".format(xvar, xstart, xend, xincr), file=sys.stderr)

need_header = True

baseenv = args.base_env if args.base_env else {}

for xval in range(xstart, xend + 1, xincr):
    env = {'JSON' : '1', xvar : str(xval)}
    env.update(baseenv)
    print("env=", env, file=sys.stderr)
    proc = subprocess.run(cmd, env=env, stdout=subprocess.PIPE, stderr=subprocess.PIPE, universal_newlines=True)
    if (proc.returncode != 0):
        print("bench failed with return ", proc.returncode, " - stderr:\n", proc.stderr)
        exit(1)
    try:
        outj = json.loads(proc.stdout)
    except:
        with open('out.json', "w") as f:
            f.write(proc.stdout)
        print("json decode failed, json written to out.json", flush=True, file=sys.stderr)
        raise

    def colname(benchname, yvar):
        return benchname if len(args.yvars) == 1 else "{} ({})".format(benchname, yvar)

    # print the value of the relevant metric for each bench in CSV format like
    # xvar bench1 bench2 ...
    benches = outj['benches']

    headers = []
    for b in benches:
        for yvar in args.yvars:
            headers.append(colname(b, yvar))

    if need_header:
        print(xvar, ",", ",".join(headers), sep='')
        need_header = False

    # the following code takes (aggregated) results for each benchmark and effectively transposes them
    # so that something like:
    #     "A" : { "Cycles" : [1, 2] },
    #     "B" : { "Cycles" : [3, 4] }
    # ends up as:
    # Cycles, A, B
    #      ?, 1, 3
    #      ?, 2, 4
    results = outj['results']
    aggregated = {}


    ylen = None
    for b in benches:
        for yvar in args.yvars:
            cname = colname(b, yvar)
            yvals = results[b][yvar]
            # ys is an array of values produced by the aggregation function
            # many aggregation functions (like min or max) produce only a single
            # value in the array, but some will have mutliple values
            ys = aggr(yvals)
            aggregated[cname] = ys
            if ylen:
                if len(ys) != ylen:
                    sys.exit("mismatched result lengths for bench {} and {}".format(b, benches[0]))
            else:
                ylen = len(ys)


    for i in range(ylen):
        print(xval,end='')
        for b in benches:
            for yvar in args.yvars:
                cname = colname(b, yvar)
                print(",", aggregated[cname][i], sep='', end='')
        print()
