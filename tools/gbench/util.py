import json
import os
import tempfile
import subprocess
import sys

# Input file type enumeration
IT_Invalid    = 0
IT_JSON       = 1
IT_Executable = 2

_num_magic_bytes = 2 if sys.platform.startswith('win') else 4
def is_executable_file(filename):
    """
    Return 'True' if 'filename' names a valid file which is likely
    an executable. A file is considered an executable if it starts with the
    magic bytes for a EXE, Mach O, or ELF file.
    """
    if not os.path.isfile(filename):
        return False
    with open(filename, 'r') as f:
        magic_bytes = f.read(_num_magic_bytes)
    if sys.platform == 'darwin':
        return magic_bytes in [
            '\xfe\xed\xfa\xce',  # MH_MAGIC
            '\xce\xfa\xed\xfe',  # MH_CIGAM
            '\xfe\xed\xfa\xcf',  # MH_MAGIC_64
            '\xcf\xfa\xed\xfe',  # MH_CIGAM_64
            '\xca\xfe\xba\xbe',  # FAT_MAGIC
            '\xbe\xba\xfe\xca'   # FAT_CIGAM
        ]
    elif sys.platform.startswith('win'):
        return magic_bytes == 'MZ'
    else:
        return magic_bytes == '\x7FELF'


def is_json_file(filename):
    """
    Returns 'True' if 'filename' names a valid JSON output file.
    'False' otherwise.
    """
    try:
        with open(filename, 'r') as f:
            json.load(f)
        return True
    except:
        pass
    return False


def classify_input_file(filename):
    """
    Return a tuple (type, msg) where 'type' specifies the classified type
    of 'filename'. If 'type' is 'IT_Invalid' then 'msg' is a human readable
    string represeting the error.
    """
    ftype = IT_Invalid
    err_msg = None
    if not os.path.exists(filename):
        err_msg = "'%s' does not exist" % filename
    elif not os.path.isfile(filename):
        err_msg = "'%s' does not name a file" % filename
    elif is_executable_file(filename):
        ftype = IT_Executable
    elif is_json_file(filename):
        ftype = IT_JSON
    else:
        err_msg = "'%s' does not name a valid benchmark executable or JSON file"
    return ftype, err_msg


def check_input_file(filename):
    """
    Classify the file named by 'filename' and return the classification.
    If the file is classified as 'IT_Invalid' print an error message and exit
    the program.
    """
    ftype, msg = classify_input_file(filename)
    if ftype == IT_Invalid:
        print "Invalid input file: %s" % msg
        sys.exit(1)
    return ftype


def load_benchmark_results(fname):
    """
    Read benchmark output from a file and return the JSON object.
    REQUIRES: 'fname' names a file containing JSON benchmark output.
    """
    with open(fname, 'r') as f:
        return json.load(f)


def run_benchmark(exe_name, benchmark_flags):
    """
    Run a benchmark specified by 'exe_name' with the specified
    'benchmark_flags'. The benchmark is run directly as a subprocess to preserve
    real time console output.
    RETURNS: A JSON object representing the benchmark output
    """
    thandle, tname = tempfile.mkstemp()
    os.close(thandle)
    cmd = [exe_name] + benchmark_flags
    print("RUNNING: %s" % ' '.join(cmd))
    exitCode = subprocess.call(cmd + ['--benchmark_out=%s' % tname])
    if exitCode != 0:
        print('TEST FAILED...')
        sys.exit(exitCode)
    json_res = load_benchmark_results(tname)
    os.unlink(tname)
    return json_res


def run_or_load_benchmark(filename, benchmark_flags):
    ftype = check_input_file(filename)
    if ftype == IT_JSON:
        return load_benchmark_results(filename)
    elif ftype == IT_Executable:
        return run_benchmark(filename, benchmark_flags)
    else:
        assert False # This branch is unreachable