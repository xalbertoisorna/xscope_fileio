# Copyright 2020-2024 XMOS LIMITED.
# This Software is subject to the terms of the XMOS Public Licence: Version 1.

import contextlib
import os
import socket
import sys
import time
import platform
import subprocess
import threading
import queue

from pathlib import Path

# How long in seconds we would expect xrun to open a port for the host app
# The firmware will have already been loaded so 5s is more than enough
# as long as the host CPU is not too busy. This can be quite long (10s+)
# for a busy CPU

XRUN_TIMEOUT = 20

def _get_host_exe():
    """ Returns the path the the host exe. Builds if the host exe doesn't exist """
    cwd = Path(__file__).parent
    HOST_PATH_git = cwd.parent / "host"
    HOST_PATH_pkg = cwd / "host"
    
    c1 = HOST_PATH_git.exists()
    c2 = HOST_PATH_pkg.exists()
    
    if c1:
        HOST_PATH = HOST_PATH_git
    elif c2:
        HOST_PATH = HOST_PATH_pkg
    else:
        assert 0, "Host exe not found. Please build the host app first"
    
    if platform.system() == 'Windows':
        return str(HOST_PATH / "xscope_host_endpoint.exe")
    else:
        return str(HOST_PATH / "xscope_host_endpoint")


@contextlib.contextmanager
def pushd(new_dir):
    """
    Context manager to temporarily change the current working directory.
    """
    previous_dir = os.getcwd()
    os.chdir(new_dir)
    try:
        yield
    finally:
        os.chdir(previous_dir)


def _print_output(x, verbose):
    if verbose:
        print(x, end="")
    else:
        print(".", end="", flush=True)


def _get_open_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(("localhost", 0))
    s.listen(1)
    port = s.getsockname()[1]
    s.close()
    return port


def _test_port_is_open(port):
    port_open = True
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        s.bind(("localhost", port))
    except OSError:
        port_open = False
    s.close()
    return port_open


class _XrunExitHandler:
    def __init__(self, adapter_id, firmware_xe):
        self.adapter_id = adapter_id
        self.firmware_xe = firmware_xe
        self.host_process = None

    def set_host_process(self, host_process):
        self.host_process = host_process

    def xcore_done(self, cmd, success, exit_code):
        if not success:
            # xrun_cmd = f"--dump-state --adapter-id {self.adapter_id} {self.firmware_xe}"
            # dump = sh.xrun(xrun_cmd.split(), _out=_print_output)
            # sys.stderr.write(dump.stdout.decode())
            self.host_process.terminate()

def old_popenAndCall(onExit, *popenArgs, **popenKWArgs):
    """
    Asynchronously runs a subprocess and executes a callback function upon completion.

    Parameters
    ----------
    onExit : callable
        Function to execute when the subprocess completes.
    *popenArgs : 
        Positional arguments passed to subprocess.Popen.
    **popenKWArgs : 
        Keyword arguments passed to subprocess.Popen.

    Returns
    -------
    subprocess.Popen
        Object representing the subprocess, returned immediately after thread starts.
    """
    def runInThread(onExit, popenArgs, popenKWArgs, q):
        proc = subprocess.Popen(*popenArgs, **popenKWArgs)
        q.put(proc)
        proc.wait()
        onExit(proc.args, proc.returncode == 0, proc.returncode)
        assert proc.returncode == 0, f'\nERROR: xrun exited with error code {proc.returncode}\n'
        return

    q = queue.Queue()
    thread = threading.Thread(target=runInThread,
                              args=(onExit, popenArgs, popenKWArgs, q))
    thread.start()

    return q.get() # returns immediately after the thread starts

def popenAndCall(onExit, *popenArgs, **popenKWArgs):
    proc = subprocess.Popen(*popenArgs, **popenKWArgs)
    return proc
    
def run_on_target(adapter_id, firmware_xe, use_xsim=False, **kwargs):
    """
    Run a target application using xrun or xsim along with a host application.

    Parameters
    ----------
    adapter_id : int
        The adapter ID for the target application.
    firmware_xe : str
        The path to the firmware executable.
    use_xsim : bool, optional
        If True, use xsim; otherwise, use xrun. Default is False.
    **kwargs
        Additional keyword arguments to be passed to subprocess.Popen.

    Returns
    -------
    int
        The return code of the host process.

    Raises
    ------
    AssertionError
        If xrun times out or if the host app exits with a non-zero return code.

    Notes
    -----
    This function starts the target application using xrun or xsim, and a host application
    to communicate with the target. The host application runs in a separate process.

    The function monitors the status of xrun to ensure it starts within a specified timeout.
    If xrun takes longer than the timeout, the function terminates the process.

    If the host application exits with a non-zero return code, the function terminates
    the xrun process and raises an AssertionError.

    Examples
    --------
    To run the target application using xrun:
    >>> run_on_target(adapter_id, 'firmware.xe')

    To run the target application using xsim:
    >>> run_on_target(None, 'firmware.xe', use_xsim=True)
    """
    port = _get_open_port()
    xrun_cmd = (
        f"--xscope-port localhost:{port} --adapter-id {adapter_id} {firmware_xe}"
    )
    xsim_cmd = ["--xscope", f"-realtime localhost:{port}", firmware_xe]

    sh_print = lambda x: _print_output(x, True)

    # Start and run in background
    exit_handler = _XrunExitHandler(adapter_id, firmware_xe)
    if use_xsim:
        print(xsim_cmd)
        xrun_proc = subprocess.Popen(['xsim'] + xsim_cmd)
    else:
        print(xrun_cmd)
        xrun_proc = popenAndCall(exit_handler.xcore_done, ["xrun"] + xrun_cmd.split(), **kwargs)

    print("Waiting for xrun", end="")
    timeout = time.time() + XRUN_TIMEOUT
    while _test_port_is_open(port):
        print(".", end="", flush=True)
        time.sleep(0.1)
        if time.time() > timeout:
            xrun_proc.terminate()
            assert 0, f"xrun timed out - took more than {XRUN_TIMEOUT} seconds to start"

    print()

    print("Starting host app...", end="\n")

    host_exe = _get_host_exe()
    host_args = f"{port}"
    host_proc = subprocess.Popen([host_exe] + host_args.split(), **kwargs)
    exit_handler.set_host_process(host_proc)
    error_message = None
    try:
        host_proc.wait()
    except KeyboardInterrupt:
        error_message = "\nStopping from keyboard interrupt\n"
    except Exception as e:
        error_message = f"\nERROR: Host app failed with error: {e}"
    finally:
        if error_message:
            print(error_message)
        host_proc.terminate()
        xrun_proc.terminate()
        host_proc.wait(timeout=10)
        xrun_proc.wait(timeout=10)
    return host_proc.returncode


def get_adapter_id():
    """
    Gets adapter ID from xrun.
    """
    try:
        xrun_out = subprocess.check_output(['xrun', '-l'], text=True, stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
        print('Error: %s' % e.output)
        assert False
    except FileNotFoundError:
        msg = ("please ensure you have XMOS tools activated in your environment")
        assert False, msg

    xrun_out = xrun_out.split('\n')
    # Check that the first 4 lines of xrun_out match the expected lines
    expected_header = ["", "Available XMOS Devices", "----------------------", ""]
    if len(xrun_out) < len(expected_header):
        raise RuntimeError(
            f"Error: xrun output:\n{xrun_out}\n"
            f"does not contain expected header:\n{expected_header}"
        )

    header_match = True
    for i, expected_line in enumerate(expected_header):
        if xrun_out[i] != expected_line:
            header_match = False
            break

    if not header_match:
        raise RuntimeError(
            f"Error: xrun output header:\n{xrun_out[:4]}\n"
            f"does not match expected header:\n{expected_header}"
        )

    try:
        if "No Available Devices Found" in xrun_out[4]:
            raise RuntimeError(f"Error: No available devices found\n")

    except IndexError:
        raise RuntimeError(f"Error: xrun output is too short:\n{xrun_out}\n")

    for line in xrun_out[6:]:
        if line.strip():
            adapterID = line[26:34].strip()
            status = line[34:].strip()
        else:
            continue
    print("adapter_id = ",adapterID)
    return adapterID
