import math

def sieve(up_to):
    if up_to < 2:
        return []
    primes = [True for _ in range(up_to+1)]
    primes[0] = primes[1] = False
    upper_limit = int(math.sqrt(up_to))
    for i in range(2, upper_limit+1):
        if not primes[i]:
            continue
        for j in range(2*i, up_to+1, i):
            primes[j] = False
    return [x for x in range(2, up_to+1) if primes[x]]
