import sys
import time


import py
import tqdm
import mmap

import subprocess

import utils


def main():
    # Main subprocess
    args = ["./avocado"]
    p = subprocess.Popen(args, stdout=subprocess.PIPE)
    p.stdout.flush()

    tmp = None
    while True:
        out = str(p.stdout.readline().strip()).split(" 0 ")
        print(len(out))
        print(out)

        if (len(out) > 20):
            if (tmp != out) and (out != b""):
                utils.read_hex(str(out))
                tmp = out

        print("\n\n")


if __name__=="__main__":
    main()
