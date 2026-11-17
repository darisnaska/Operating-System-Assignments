import ctypes
import os

_here = os.path.dirname(os.path.abspath(__file__))
_lib = ctypes.CDLL(os.path.join(_here, 'libcollatz.so'))
_lib.collatz.argtypes = [ctypes.c_longlong]
_lib.collatz.restype = ctypes.c_int

def collatz(n: int) -> int:
    return _lib.collatz(n)

def main() -> None:
    lower: int = 1
    upper: int = 1000000
    longest: int = 0
    length: int = 0
    for n in range(lower, upper):
        l = collatz(n)
        if l > length:
            longest = n
            length = l
    print(f'longest collatz sequence in [{lower}:{upper}) is at {longest} (length {length})')

if __name__ == '__main__':
    main()
