#!/usr/bin/env python3
# coding: utf-8

import numpy as np
import matplotlib.pyplot as plt
from argparse import ArgumentParser
import os

parser = ArgumentParser(usage="Compare 2 files as outputted by sfizz_plot_lfo")
parser.add_argument("file", help="The file to test")
parser.add_argument("reference", help="The reference file")
args = parser.parse_args()

assert os.path.exists(args.file), "The file to test does not exist"
assert os.path.exists(args.reference), "The reference file does not exist"

reference_data = np.loadtxt(args.reference)
data = np.loadtxt(args.file)

assert reference_data.shape == data.shape, "The shapes of the data and reference are different"

n_samples, n_lfos = reference_data.shape
n_lfos -= 1  # The first column is the time

if n_lfos == 1:
    fig, ax = plt.subplots(figsize=(20, 10))
    ax.plot(reference_data[:, 0], reference_data[:, 1])
    ax.plot(data[:, 0], data[:, 1])
    ax.grid()
elif n_lfos > 4:
    n_cols = 4
    n_rows = n_lfos // n_cols
    print(n_rows, n_cols)
    fig, ax = plt.subplots(n_rows, n_cols, figsize=(20, 10))
    for i in range(n_rows):
        for j in range(n_cols):
            lfo_index = 1 + i * 4 + j
            ax[i, j].plot(reference_data[:, 0], reference_data[:, lfo_index])
            ax[i, j].plot(data[:, 0], data[:, lfo_index])
            ax[i, j].set_title("LFO {}".format(lfo_index))
            ax[i, j].grid()
else:
    fig, ax = plt.subplots(1, n_lfos, figsize=(20, 10))
    for i in range(n_lfos):
        lfo_index = i + 1
        ax[i].plot(reference_data[:, 0], reference_data[:, lfo_index])
        ax[i].plot(data[:, 0], data[:, lfo_index])
        ax[i].set_title("LFO {}".format(lfo_index))
        ax[i].grid()

plt.show()
