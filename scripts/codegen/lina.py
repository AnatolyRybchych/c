#!/bin/python3

import sys
import os.path
import itertools

parent_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
sys.path.insert(0, parent_dir)

import basics

DIR=os.path.dirname(sys.argv[0])
HEADER_FILE = f'{basics.INCLUDE_DIR}/geometry/lina.h'
SRC_FILE = f'{basics.SRC_DIR}/geometry/lina.c'

default_sizes = [2, 3, 4]
default_ops = ['', 'add', 'sub', 'mul', 'div', 'lerp', 'min', 'max', 'clamp', 'sqrmag', 'mag', 'sqrdst', 'dst', 'equ']

types = [
    {
        'type': 'float',
        'sufix': 'f',
        'sizes': default_sizes,
        'ops': default_ops
    },
    {
        'type': 'int',
        'sufix': 'i',
        'sizes': default_sizes,
        'ops': default_ops
    },
    {
        'type': 'unsigned',
        'sufix': 'u',
        'sizes': default_sizes,
        'ops': default_ops
    },
]

dimentions = {
    2: ['x', 'y'],
    3: ['x', 'y', 'z'],
    4: ['x', 'y', 'z', 'w'],
}

Vec = lambda type, size: f'MC_Vec{size}{type["sufix"]}'
vec = lambda type, size: f'mc_vec{size}{type["sufix"]}'
IDENT = '    '

def binop_decl(op):
    return lambda type, size: [
        f'{Vec(type, size)} {vec(type, size)}{op}({Vec(type, size)} lhs, {Vec(type, size)} rhs)'
    ]

def binop_body(op, cop):
    return lambda type, size:[
        f'{IDENT}return {vec(type, size)}(' + ",".join([f'lhs.{dim} {cop} rhs.{dim}' for dim in dimentions[size]]) + ');'
    ]

ops = {
    '': {
        'decl': lambda type, size: [
            f'{Vec(type, size)} {vec(type, size)}(' + ', '.join([
                f'{type["type"]} {dim}' for dim in dimentions[size]
            ]) + ')'
        ],
        'body': lambda type, size: [
            f'{IDENT}return ({Vec(type, size)}){{' + ", ".join([f'.{dim} = {dim}' for dim in dimentions[size]]) + f'}};'
        ]
    },
    'add': { 'decl': binop_decl('_add'), 'body': binop_body('_add', '+')},
    'sub': { 'decl': binop_decl('_sub'), 'body': binop_body('_sub', '-')},
    'mul': { 'decl': binop_decl('_mul'), 'body': binop_body('_mul', '*')},
    'div': { 'decl': binop_decl('_div'), 'body': binop_body('_add', '/')},
    'min': {
        'decl': binop_decl('_min'),
        'body':  lambda type, size: [
            f'{IDENT}return {vec(type, size)}(' + f','.join([
                f'\n{IDENT*2}lhs.{dim} < rhs.{dim} ? lhs.{dim} : rhs.{dim}' for dim in dimentions[size]
            ]) + ');'
        ]
    },
    'max': {
        'decl': binop_decl('_max'),
        'body':  lambda type, size: [
            f'{IDENT}return {vec(type, size)}(' + f','.join([
                f'\n{IDENT*2}lhs.{dim} > rhs.{dim} ? lhs.{dim} : rhs.{dim}' for dim in dimentions[size]
            ]) + ');'
        ]
    },
    'equ': {
        'decl': lambda type, size: [
            f'bool {vec(type, size)}_equ({Vec(type, size)} lhs, {Vec(type, size)} rhs)'
        ],
        'body':  lambda type, size: [
            f'{IDENT}return ' + ' && '.join([f'lhs.{dim} == rhs.{dim}' for dim in dimentions[size]]) + ';'
        ]
    },
    'lerp': {
        'decl': lambda type, size: [
            f'{Vec(type, size)} {vec(type, size)}_lerp({Vec(type, size)} beg, {Vec(type, size)} end, float progress)'
        ],
        'body': lambda type, size: [
            f'{IDENT}return {vec(type, size)}(' + f','.join([
                f'\n{IDENT*2}beg.{dim} + (end.{dim} - beg.{dim}) * progress' for dim in dimentions[size]
            ]) + ');'
        ]
    },
    'clamp': {
        'decl': lambda type, size: [
            f'{Vec(type, size)} {vec(type, size)}_clamp({Vec(type, size)} val, {Vec(type, size)} min, {Vec(type, size)} max)'
        ],
        'body': lambda type, size: [
            f'{IDENT}return {vec(type, size)}_min(max, {vec(type, size)}_max(min, val));'
        ]
    },
    'sqrmag': {
        'decl': lambda type, size: [
            f'float {vec(type, size)}_sqrmag({Vec(type, size)} val)'
        ],
        'body': lambda type, size: [
            f'{IDENT}return ('+ '+ '.join([f'val.{dim} * val.{dim}' for dim in dimentions[size]]) +');',
        ]
    },
    'mag': {
        'decl': lambda type, size: [
            f'float {vec(type, size)}_mag({Vec(type, size)} val)'
        ],
        'body': lambda type, size: [
            f'{IDENT}return sqrt({vec(type, size)}_sqrmag(val));',
        ]
    },
    'sqrdst': {
        'decl': lambda type, size: [
            f'float {vec(type, size)}_sqrdst({Vec(type, size)} vec1, {Vec(type, size)} vec2)'
        ],
        'body': lambda type, size: [
            f'{IDENT}return {vec(type, size)}_sqrmag({vec(type, size)}_sub(vec2, vec1));',
        ]
    },
    'dst': {
        'decl': lambda type, size: [
            f'float {vec(type, size)}_dst({Vec(type, size)} vec1, {Vec(type, size)} vec2)'
        ],
        'body': lambda type, size: [
            f'{IDENT}return {vec(type, size)}_mag({vec(type, size)}_sub(vec2, vec1));',
        ]
    },
}

HEADER_GUARD = 'MC_GEOMETRY_LINA_H'
LINA_INCLUDE = '<mc/geometry/lina.h>'

def main():
    hdr = []
    src = []

    hdr += [f'#ifndef {HEADER_GUARD}']
    hdr += [f'#define {HEADER_GUARD}']
    hdr += [f'']
    hdr += [f'#include <math.h>']
    hdr += [f'#include <stddef.h>']
    hdr += [f'#include <stdbool.h>']
    hdr += [f'']

    hdr += ['float mc_lerpf(float beg, float end, float progress);']
    hdr += ['float mc_clampf(float val, float min, float max);']

    types_sizes = [(t, s) for t in types for s in t['sizes']]
    types_sizes_ops = [(t, s, o) for t in types for s in t['sizes'] for o in t['ops']]

    for type, size in types_sizes:
        sufix = type['sufix']
        hdr += [f'typedef struct {Vec(type, size)} {Vec(type, size)};']

    hdr += [f'']

    for type, size, op in types_sizes_ops:
        hdr += ops[op]['decl'](type, size)
        hdr[-1] += ';'

    hdr += [f'']

    for type, size in types_sizes:
        sufix = type['sufix']
        hdr += [
            f'struct {Vec(type, size)}{{',
            f'{IDENT}{type["type"]} ' + ', '.join(dimentions[size]) + ';',
            f'}};',
            f''
        ]

    hdr += [f'']
    hdr += [f'#endif //{HEADER_GUARD}']


    src += [f'#include {LINA_INCLUDE}']
    src += [f'']

    src += [
        'float mc_lerpf(float beg, float end, float progress){',
        f'{IDENT}return beg + (end - beg) * progress;',
        '}',
        '',
    ]
    
    src += [
        'float mc_clampf(float val, float min, float max){',
        f'{IDENT}return val < min ? min : val > max ? max : val;',
        '}',
        '',
    ]

    for type, size, op in types_sizes_ops:
        src += ops[op]['decl'](type, size)
        src[-1] += '{'
        src += ops[op]['body'](type, size)
        src += ['}', '']

    with open(HEADER_FILE, 'w') as file:
        file.write('\n'.join(hdr))

    with open(SRC_FILE, 'w') as file:
        file.write('\n'.join(src))

if __name__ == '__main__':
    main()
