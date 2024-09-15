#!/bin/python3

import sys
import os.path

WORKDIR = os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
INCLUDE_DIR = f'{WORKDIR}/include/mc/'
SRC_DIR = f'{WORKDIR}/src'

if __name__ == '__main__':
    print(f'WORKDIR: {WORKDIR}')