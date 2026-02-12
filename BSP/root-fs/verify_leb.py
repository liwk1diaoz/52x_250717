#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
import pyfdt
import json
import argparse
import ubi_max_leb
from pyfdt import FdtBlobParse

# find partition size by label name
def find_partition_size(fdt_root, label):
    fdt_iter = fdt_root.walk()
    for node in fdt_iter:
        currpath = r"" + node[0]
        if "nand@" in currpath or "nor@" in currpath:
            currnode = node[1]
            if not isinstance(currnode, pyfdt.FdtNode):
                continue
            subnode_iter = currnode.walk()
            for subnode in subnode_iter:
                currsubnode = subnode[1]
                if not isinstance(currsubnode, pyfdt.FdtNode):
                    continue
                try:
                    label_index = currsubnode.index("label")
                except ValueError:
                    continue
                lable_name = currsubnode[label_index]
                if lable_name.strings[0] == label:
                    reg_index = currsubnode.index("reg")
                    reg = currsubnode[reg_index]
                    partition_size = int(reg.words[2])<<32 | int(reg.words[3])
                    return partition_size
    return None

def parse_args(argv):
    parser = argparse.ArgumentParser(description='fdt find partition size')
    parser.add_argument('-f', '--file',metavar='.dtb/.bin', required=True,
                    dest='file', action='store',
                    help='fdt binary file')
    parser.add_argument('-l', '--label',metavar='partition_label', required=True,
                    dest='label', action='store',
                    help='the label of partition')
    parser.add_argument('-b', '--leb',metavar="mtd_cfg's leb", required=True,
                    dest='leb', action='store',
                    help='the label of partition')
    args = parser.parse_args()
    return args

def main(argv):
    if 0:
        argv.append(r"-f")
        argv.append(r"d:\tmp\nvt-na51055-evb.bin")
        argv.append(r"-l")
        argv.append(r"rootfs2")

    args = parse_args(argv)

    if not os.path.isfile(args.file):
        print("cannot find {}".format(args.file), file=sys.stderr)
        return -1

    with open(args.file, 'rb') as infile:
        dtb = FdtBlobParse(infile)

    fdt_root = dtb.to_fdt().get_rootnode()
    partition_size = find_partition_size(fdt_root, args.label)
    if partition_size is None:
        print("cannot find label:'{}' in storage partition".format(args.label), file=sys.stderr)
        return -1

    expected_leb = int(ubi_max_leb.get_ubifs_max_leb(partition_size))
    mtdcfg_leb = int(args.leb)

    if expected_leb != mtdcfg_leb:
        print("leb not matched. expected:{} vs mtdcfg:{} on {} partition size: 0x{:X}".format(expected_leb, mtdcfg_leb, args.label, partition_size), file=sys.stderr)
        return -1

    # output partition size for Makefile
    print("leb verify ok, leb:{} on {} partition size: 0x{:X}".format(mtdcfg_leb, args.label, partition_size))
    return 0


if __name__ == '__main__':
    try:
        er = main(sys.argv)
    except Exception as exp:
        er = -1
        print(exp, file=sys.stderr)
        raise exp
    sys.exit(er)

