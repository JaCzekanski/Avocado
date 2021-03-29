import os
import re
import mmap

from textwrap import wrap
from google_trans_new import google_translator

translator = google_translator()


def read_hex(hexf):
    hexf = hexf.replace(" ", "")
    hexf = hexf.replace("8200", "")
    hexf = hexf.replace("20", "")
    hexf = hexf.replace("00", "")

    for i in range(4):
        out = []
        for k in [hexf[c:c+4] for c in range(i, len(hexf), 4)]:
            try:
                kana = bytes.fromhex(k).decode("shift_jisx0213", "ignore")
                out.append(kana)

            except:
                continue

        out = ''.join(out)
        # out_trans = translator.translate(out, lang_tgt='en')

        # print("Translated:", out_trans)
        print("Original:", out)
        print()


def read_hex_file(filename, addr):
    f_ptr = os.open(filename, os.O_RDWR)

    mm = mmap.mmap(f_ptr, 0, prot=mmap.PROT_READ)
    mm.seek(addr)
    line = mm.readline()

    out = line.decode("shift-jis", "ignore")
    # out = translator.translate(out, lang_tgt='en')

    return out


if __name__=="__main__":
    text = input()
    print(read_hex(text))
