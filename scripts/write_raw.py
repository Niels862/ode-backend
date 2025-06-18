import lib
import sys


def main():
    if len(sys.argv) != 3:
        print(f"usage: {sys.argv[0]} (C project root) (dest)")
        exit(1)
    
    cfg = lib.extract_configuration(sys.argv[1])

    with open(sys.argv[2], "w") as file:
        data = cfg.banks.sram_banks
        file.write(" ".join([ " ".join( str(byte) for byte in bank ) for bank in data ]))


if __name__ == "__main__":
    main()
