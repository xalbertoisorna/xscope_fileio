# Copyright 2021-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.
#We assume that the Xscope FileIO Python library has been installed via pip beforehand and is available to import. Please see readme for instuctions.
import subprocess
import numpy as np
import xscope_fileio
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description="Run xscope_fileio_close.xe")
parser.add_argument("--adapter-id", help="adapter_id to use", required=True)
try:
    args = parser.parse_args()
    adapter_id = args.adapter_id
    print(f"Using adapter ID: {adapter_id}")
except SystemExit:
    print('Note: run "xrun -l" to see available adapters')
    adapter_id = "EHV92U6D"
    #exit(1)

firmware_xe = (Path(__file__).parent / "bin" / "stdio_vs_xscope.xe").absolute()
xscope_fileio.run_on_target(adapter_id, firmware_xe, use_xsim=False)
print("Example run successfully!")
