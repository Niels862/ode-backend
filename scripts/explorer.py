import lib
import sys


def main():
    for name in sys.argv[1:]:
        cfg = lib.extract_configuration(name)
        cfg.banks.print_sram_all()
        if not sum(sum(bank) for bank in cfg.banks.lut_banks) == 0:
            print("LUT Banks non-empty")


if __name__ == "__main__":
    main()
