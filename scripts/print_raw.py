import sys


def main():
    with open(sys.argv[1], "r") as file:
        data = list(map(int, file.read().split()))

        for i in range(11):
            for j in range(32):
                print(f"{data[32 * i + j]:02X}", end=" ")
            print()


if __name__ == "__main__":
    main()
