import matplotlib.pyplot as plt
import numpy as np

global x, labels
def init():
    global x
    plt.figure(figsize=(11,8), dpi=400)

    x = [i for i in range(1, 7)]
    # y = [5000 * i for i in range(1, 6)]
    labels = [str(2**i) + " (" + str(2**i * 4) + ")" for i in range(0, len(x))]
    plt.xticks(x, labels, fontsize=25)
# plt.yticks(y, fontsize=20)

absolute = True

cpp = dsl = dft = None
GEMM = True
fname=""

def cannon():
    global cpp, dsl, dft, GEMM, fname
    GEMM = True
    cpp = [27117.9661,9292.142102,13372.16883,3998.332924,12376.63895,3958.998471,8899.986094]
    dsl = [27117.9661,9765.371609,14828.359592,4021.070911,12931.82461,3992.706196,10914.01774]
    dft = [7637.040573,4326.265013,6168.003084,3216.056572,6646.54689,2987.807476,6257.619164]
    fname="perfcannon"

def pumma():
    global cpp, dsl, dft, GEMM, fname
    GEMM = True
    cpp = [27210.20408,16336.72637,16023.83575,6770.36685,12648.14229,3099.967942,9151.326232]
    dsl = [27026.35135,16138.63482,15391.82299,6799.869576,13448.11935,3039.176093,9732.329684]
    dft = [7622.486899,5187.154062,5818.109091,0,5716.802144,0,6570.484061]
    fname="perfpumma"
    plt.text(3.75, 4300, "OOM", fontsize=20, color='darkorange')
    plt.text(5.75, 6900, "OOM", fontsize=20, color='darkorange')

def summa():
    global cpp, dsl, dft, GEMM, fname
    GEMM = True
    cpp = [26844.96644,15756.5223,15678.39295,6136.017902,12576.06603,3270.694711,8448.818482]
    dsl = [27256.55877,15205.89114,14492.57246,6152.703199,12673.18812,3357.943978,8673.822593]
    dft = [7607.988588,4796.770409,6092.840823,0,6011.609994,0,5418.43119]
    fname="perfsumma"
    plt.text(3.75, 4300, "OOM", fontsize=20, color='darkorange')
    plt.text(5.75, 6500, "OOM", fontsize=20, color='darkorange')

def solomonik():
    global cpp, dsl, dft, GEMM, fname
    GEMM = True
    cpp = [28267.84452,14010.13376,11790.56743,7116.391261,10657.71857,5743.173047,8850.753699]
    dsl = [28217.98942,14053.64349,12038.97667,6581.316467,10435.28453,5410.165823,10717.54166]
    dft = [28118.80492,14311.42696,16251.70137,8217.383699,11506.58037,5102.128871,0]
    fname="perfsolomonik"

def johnson():
    global cpp, dsl, dft, GEMM, fname
    GEMM = True
    cpp = [27680.96886,15154.96719,10796.08637,7473.67994,9400.6463]
    dsl = [27728.94281,15144.8233,10869.42935,6842.021776,9396.505652]
    dft = [29519.5572,12812.21179,18680.44367,10110.17248,0]
    fname="perfjohnson"
    plt.text(4.75, 12000, "OOM", fontsize=20, color='darkorange')

def cosma():
    global cpp, dsl, dft, GEMM, fname
    GEMM = True
    cpp = [21857.37705,15175.29579,13366.58312,8527.064636,9421.404388]
    dsl = [22252.57302,15054.13574,13417.02306,8468.026204,9407.555492]
    dft = [20024.53066,12848.58945,15332.82223,0,0]
    fname="perfcosma"
    plt.text(3.8, 13000, "OOM", fontsize=20, color='darkorange')
    plt.text(4.8, 13000, "OOM", fontsize=20, color='darkorange')

def circuit():
    global cpp, dsl, dft, GEMM, fname
    GEMM = False
    wires = 20000 * 40 * 50
    ctime = [1.1964,1.21,1.2144,1.3848,1.2278,1.3448,1.3484]
    dtime = [1.1994,1.2046,1.209,1.3782,1.2272,1.3404,1.3488]
    ftime = [1.0418,1.0952,1.1082,1.1184,1.1304,1.14,1.1484]
    cpp = [wires / i / 1e7 for i in ctime]
    dsl = [wires / i / 1e7 for i in dtime]
    dft = [wires / i / 1e7 for i in ftime]
    y = [2 + i * 0.5 for i in range(0, 7)]
    plt.ylim([2, 5])
    plt.yticks(y, fontsize=20)
    plt.xlabel("Nodes (GPUs)", fontsize=20)
    plt.ylabel("Thoughput Per Node ($10^{7}$ wires/s)", fontsize=20)
    fname = "perfcircuit"

def stencil():
    global cpp, dsl, dft, GEMM, fname
    GEMM = False
    cells = 4 * 50 * 15000 * 15000
    ctime = [0.5578,0.5776,0.5952,0.6052,0.6112,0.6204,0.6312]
    dtime = [0.558,0.5774,0.5954,0.6052,0.6126,0.6202,0.6306]
    ftime = [0.5582,0.586,0.5954,0.6016,0.6316,0.6396,0.6518]
    cpp = [cells / i / 1e10 for i in ctime]
    dsl = [cells / i / 1e10 for i in dtime]
    dft = [cells / i / 1e10 for i in ftime]
    y = [6.5 + i * 0.5 for i in range(0, 6)]
    plt.ylim([6.5, 8.5])
    plt.yticks(y, fontsize=20)
    plt.xlabel("Nodes (GPUs)", fontsize=20)
    plt.ylabel("Thoughput Per Node ($10^{10}$ cells/s)", fontsize=20)
    fname = "perfstencil"

def pennant():
    global cpp, dsl, dft, GEMM, fname
    GEMM = False
    zones = 30 * 320 * 92160
    ctime = [1.2746,1.3586,1.3804,1.4068,1.4818,1.5388,1.5562]
    dtime = [1.2758,1.3892,1.401,1.4342,1.4766,1.5366,1.5528]
    ftime = [1.2718,1.3632,1.3996,1.4394,1.5116,1.5446,1.5788]
    cpp = [zones / i / 1e8 for i in ctime]
    dsl = [zones / i / 1e8 for i in dtime]
    dft = [zones / i / 1e8 for i in ftime]
    y = [5+i*0.5 for i in range(0, 7)]
    plt.ylim([5, 8])
    plt.yticks(y, fontsize=20)
    plt.xlabel("Nodes (GPUs)", fontsize=20)
    plt.ylabel("Thoughput Per Node ($10^{8}$ zones/s)", fontsize=20)
    fname = "perfpennant"

def baseline():
    if absolute:
        print("BENCH," + ",".join([str(item) if item > 0 else "N/A" for item in (units(cpp) if GEMM else cpp)] + ["N/A"] * (len(x) - len(cpp)) ))
    else:
        print(",".join(["1.0"] * (len(cpp) + 1) + ["N/A"] * (len(x) - len(cpp)) ))

def compute(bench, a, b):
    if absolute:
        print(bench + ",".join([str(a[i]) if a[i] != 0 else "N/A" for i in range(len(a)) ]
                           + ["N/A"] * (len(x) - len(cpp)) ))
    else:
        print(bench + ",".join([str(a[i] / b[i]) if ( b[i] != 0 and a[i] != 0) else "N/A" for i in range(len(a)) ]
                           + ["N/A"] * (len(x) - len(cpp)) ))

def units(a):
    return [i / 1e3 for i in a]

def numbers():
    circuit(), baseline(), compute("circuitdsl,", dsl, cpp), compute("circuitdft,", dft, cpp)
    stencil(), baseline(), compute("stencildsl,", dsl, cpp), compute("stencildft,", dft, cpp)
    pennant(), baseline(), compute("pennantdsl,", dsl, cpp), compute("pennantdft,", dft, cpp)
    # throughput
    cannon(), baseline(), compute("cannondsl,", units(dsl), units(cpp)), compute("cannondft,", units(dft), units(cpp))
    pumma(), baseline(), compute("pummadsl,", units(dsl), units(cpp)), compute("pummadft,", units(dft), units(cpp))
    summa(), baseline(), compute("summadsl,", units(dsl), units(cpp)), compute("summadft,", units(dft), units(cpp))
    solomonik(), baseline(), compute("solomonikdsl,", units(dsl), units(cpp)), compute("solomonikdft,", units(dft), units(cpp))
    johnson(), baseline(), compute("johnsondsl,", units(dsl), units(cpp)), compute("johnsondft,", units(dft), units(cpp))
    cosma(), baseline(), compute("cosmadsl,", units(dsl), units(cpp)), compute("cosmadft,", units(dft), units(cpp))
    exit()

# cannon()
# pumma()
# summa()
# solomonik()
# johnson()
# cosma()

# circuit()
# stencil()
# pennant()

# numbers()

def draw(f):
    init()
    f()
    global cpp, dsl, dft, GEMM, fname, x
    cpp, dsl, dft = cpp[:-1], dsl[:-1], dft[:-1]
    length = len(cpp)
    x = np.array(x)
    # cpp = np.array(cpp)
    dsl = np.array(dsl)
    dft = np.array(dft)

    # plt.plot(x[:length][cpp > 0], cpp[cpp > 0], "--x", label="Hand-tuned C++", linewidth=3, markersize=20)
    plt.plot(x[:length][dsl > 0], dsl[dsl > 0], "--o", label="Algorithm Specification", linewidth=3, markersize=20)

    if GEMM:
        plt.plot(x[:length][dft>0], dft[dft>0], "--^", label="Runtime Heuristics", linewidth=3, markersize=20)
        y = [5000 * i for i in range(0, 7)]
        plt.ylim([0, 30000])
        plt.yticks(y, fontsize=30)
        plt.xlabel("Nodes (GPUs)", fontsize=30)
        plt.ylabel("GFLOP/s Per Node", fontsize=30)
    # plt.plot(x, full_db, "--+", label="full - db")
    else:
        plt.plot(x[:length][dft>0], dft[dft>0], "--^", label="Default Maple", linewidth=3, markersize=20)


    plt.legend(fontsize=30)
    # plt.show()
    plt.savefig(f"{fname}.pdf", format="pdf", bbox_inches='tight')

draw(cannon)
draw(pumma)
draw(summa)
# draw(solomonik)
# draw(johnson)
# draw(cosma)

# draw(circuit)
# draw(stencil)
# draw(pennant)