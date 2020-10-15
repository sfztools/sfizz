#!/usr/bin/env python3
# coding: utf-8

import numpy as np
from argparse import ArgumentParser
import os

parser = ArgumentParser(usage="Compare 2 files as outputted by sfizz_plot_lfo")
parser.add_argument("file", help="The file to test")
parser.add_argument("reference", help="The reference file")
parser.add_argument("--threshold", type=float, default=0.001, help="Mean squared error threshold")
args = parser.parse_args()

assert os.path.exists(args.file), "The file to test does not exist"
assert os.path.exists(args.reference), "The reference file does not exist"

reference_data = np.loadtxt(args.reference)
data = np.loadtxt(args.file)

assert reference_data.shape == data.shape, "The shapes of the data and reference are different"

mean_squared_error = np.mean((data - reference_data) ** 2)
print("MSE difference:", mean_squared_error)

if (mean_squared_error > args.threshold):
    exit(-1)

