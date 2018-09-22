import bisect
import os.path
import time


class SimpleTimer():
    def __init__(self):
        self.start = time.time()


class BadFiletype(BaseException):
    """"""


class BufferChanged(BaseException):
    """"""


def do_set_base(echo, error, settings_file, args):
    try:
        with open(settings_file, 'r') as fp:
            projects = {
                p: int(run) for p, run in
                [j.split('\t') for j in [i.rstrip() for i in fp]]
            }
    except FileNotFoundError:
        projects = {}

    with open(settings_file, 'a') as fp:
        path, run_ctags = args
        path = os.path.realpath(path)
        if os.path.exists(path):
            if path not in projects:
                fp.write("%s\t%d\n" % (path, run_ctags))
                echo("Saved directory '%s' as a project base." % path, force=True)
            else:
                error("Error: directory '%s' is already saved as a project base." % path)
        else:
            error("Error: directory '%s' does not exist." % path)


def do_remove_base(echo, error, settings_file, args):
    try:
        with open(settings_file, 'r') as fp:
            projects = {
                p: int(run) for p, run in
                [j.split('\t') for j in [i.rstrip() for i in fp]]
            }
    except FileNotFoundError:
        return

    path = os.path.realpath(args[0])

    if path in projects:
        projects.pop(path)
        echo("Removed directory '%s' from project list." % path, force=True)
        with open(settings_file, 'w') as fp:
            for item in projects.items():
                fp.write("%s\t%d\n" % item)
    else:
        echo("Error: directory '%s' is not a known project base." % path, force=True)


def bindex(a, x):
    """Locate the leftmost value exactly equal to x"""
    i = bisect.bisect_left(a, x)
    if i != len(a) and a[i] == x:
        return i
    return (-1)


def try_dict_get(dictionary, item):
    try:
        return dictionary[item]
    except KeyError:
        return None
