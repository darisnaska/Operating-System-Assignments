## Project Euler 14 written in plain Python

def collatz(n: int) -> int:
    i: int = 1
    while n != 1:
        if n % 2 == 0:
            n = n // 2
        else:
            n = 3 * n + 1
        i += 1
    return i

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
