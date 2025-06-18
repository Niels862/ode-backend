import clang.cindex
from sys import argv
from typing import List
import os
import sys


clang.cindex.Config.set_library_file("/usr/lib/x86_64-linux-gnu/libclang-14.so.14.0.0")


class MemoryBanks:
    def __init__(self):
        self.sram_banks: List[List[int]] = [
            [ 0 for _ in range(32) ] for _ in range(11)
        ]
        self.lut_banks: List[List[int]] = [
            [ 0 for _ in range(32) ] for _ in range(11)
        ]

    def write(self, bank: int, byte: int, data: int):
        bank = bank + byte // 32
        byte = byte % 32

        if bank <= 0x0A:
            self.sram_banks[bank][byte] = data
        else:
            self.lut_banks[bank - 0x10][byte] = data
        
    def read(self, bank: int, byte: int):
        bank = bank + byte // 32
        byte = byte % 32

        if bank <= 0x0A:
            return self.sram_banks[bank][byte]
        else:
            return self.lut_banks[bank - 0x10][byte]

    def print_mem(self, mem, start, filter):
        print("    " + " ".join(f"{i:02x}" for i, _ in enumerate(mem[0])))
        for i, row in enumerate(mem):
            if filter(row):
                idx = start + i
                print(f"{idx:02X}: " + " ".join(("XX" if hx == -1 else f"{hx:02X}") if hx else ".." for hx in row))
 
    def print_sram_all(self):
        self.print_mem(self.sram_banks, 0, lambda _: True)

    def print_lut_all(self):
        self.print_mem(self.lut_banks, 0x10, lambda _: True)        

    def print_all(self):
        self.print_sram_all()
        self.print_lut_all()


class ConfigurationData:
    def __init__(self, data: List[int]):
        self.data: List[int] = data
        self.banks = MemoryBanks()
        self.cursor: int = 0
        self.dev_id: int = 0
        self.addr1: int = 0
        self.parse()

    def next(self):
        if self.cursor >= len(self.data):
            raise RuntimeError("Unexpected data termination")
        self.cursor += 1
        return self.data[self.cursor - 1]

    def next_n(self, n):
        x = 0
        for i in range(n):
            x = x << 8 | self.next()
        return x

    def curr(self):
        return self.data[self.cursor]

    def parse(self):
        if self.next() != 0xD5:
            raise RuntimeError(f"Expected SYNC byte")
        self.dev_id = self.next_n(4)
        self.addr1 = self.next()
        self.next()  # Control, skipped

        data_follows: bool = True
        while data_follows:
            ctrl = self.next()
            byte_addr = ctrl & 0x1F
            use_crc = (ctrl >> 5) & 1
            data_follows = (ctrl >> 6) & 1

            bank_addr = self.next()

            cnt = self.next()
            if cnt == 0:
                cnt = 256

            for i in range(cnt):
                self.banks.write(bank_addr, byte_addr + i, self.next())

            if use_crc:
                crc = self.next_n(2)
            elif self.next() != 0x2A:
                raise RuntimeError("Expected DATA BLOCK END byte")

    def __repr__(self):
        return f"<Configuration object for {self.dev_id:08X}>"


def extract_integer_literal(node):
    if node.kind == clang.cindex.CursorKind.INTEGER_LITERAL:
        tokens = list(node.get_tokens())
        if tokens:
            return int(tokens[0].spelling, 0)
    
    if node.kind == clang.cindex.CursorKind.UNEXPOSED_EXPR:
        for child in node.get_children():
            return extract_integer_literal(child)


def parse(fn) -> clang.cindex.TranslationUnit:
    index = clang.cindex.Index.create()
    return index.parse(fn, args=["-std=c11"])


def find_integer_array(fn, name):
    tu = parse(fn)
    for node in tu.cursor.walk_preorder():
        if node.kind == clang.cindex.CursorKind.VAR_DECL and node.spelling == name:
            for child in node.get_children():
                if child.kind == clang.cindex.CursorKind.INIT_LIST_EXPR:
                    byte_values = []
                    for value in child.get_children():
                        byte_values.append(extract_integer_literal(value))
                    return byte_values


def extract_configuration(name) -> ConfigurationData:
    name = name.strip("/")
    cache = os.path.join(".cache", name)
    if os.path.exists(cache):
        with open(cache, "r") as file:
            data = eval(file.read())
    else:
        path = os.path.join(name, "CCode/ApiCode.c")
        if not os.path.exists(path): 
            path = name
        if not os.path.exists(path) or not path.endswith(".c"):
            raise FileNotFoundError("path")
        data = find_integer_array(path, "an_FPAA1_PrimaryConfigInfo")
        with open(cache, "w") as file:
            file.write(repr(data))
    return ConfigurationData(data)
