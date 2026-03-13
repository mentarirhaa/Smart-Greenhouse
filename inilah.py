# ================= FUZZY MEMBERSHIP =================

def mu_dingin(x):
    if x <= 15: return 1
    if x >= 22: return 0
    return (22 - x) / 7.0

def mu_normal(x):
    if x <= 18 or x >= 30: return 0
    if x <= 24: return (x - 18) / 6.0
    return (30 - x) / 6.0

def mu_panas(x):
    if x <= 26: return 0
    if x >= 40: return 1
    return (x - 26) / 14.0


def mu_kering(x):
    if x <= 0: return 1
    if x >= 45: return 0
    return (45 - x) / 45.0

def mu_normal_tanah(x):
    if x <= 40 or x >= 75: return 0
    if x <= 57.5: return (x - 40) / 17.5
    return (75 - x) / 17.5

def mu_basah(x):
    if x <= 65: return 0
    if x >= 100: return 1
    return (x - 65) / 35.0


def mu_cahaya_rendah(x):
    if x <= 0: return 1
    if x >= 6000: return 0
    return (6000 - x) / 6000.0

def mu_cahaya_normal(x):
    if x <= 5000 or x >= 10000: return 0
    if x <= 7500: return (x - 5000) / 2500.0
    return (10000 - x) / 2500.0

def mu_cahaya_tinggi(x):
    if x <= 9000: return 0
    if x >= 20000: return 1
    return (x - 9000) / 11000.0


# ================= FUZZY OUTPUT =================

def fuzzyFan(t):

    d = mu_dingin(t)
    n = mu_normal(t)
    p = mu_panas(t)

    den = d + n + p

    if den == 0:
        return 0

    return (d*0 + n*50 + p*100) / den


def fuzzyLamp(lux):

    r = mu_cahaya_rendah(lux)
    n = mu_cahaya_normal(lux)
    t = mu_cahaya_tinggi(lux)

    den = r + n + t

    if den == 0:
        return 0

    return (r*255 + n*120 + t*0) / den


def fuzzyPump(s):

    k = mu_kering(s)
    n = mu_normal_tanah(s)
    b = mu_basah(s)

    den = k + n + b

    if den == 0:
        return 0

    return (k*15 + n*8 + b*0) / den


# ================= TEST =================

suhu = 29.8
lux = 5403.53
soil = 35

fan = fuzzyFan(suhu)
lamp = fuzzyLamp(lux)
pump = fuzzyPump(soil)

print("Fan PWM:", fan)
print("Lamp PWM:", lamp)
print("Pump Duration:", pump)