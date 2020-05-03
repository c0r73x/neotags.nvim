import bisect
import json
import re
import os.path


def do_set_base(dia, settings_file, args):
    try:
        with open(settings_file, 'r') as fp:
            projects = json.load(fp)
    except (json.JSONDecodeError, FileNotFoundError):
        projects = {}

    path, run_ctags = args
    path = os.path.realpath(path)
    if os.path.exists(path):
        if path not in projects:
            projects[path] = {
                'run': run_ctags,
            }
            with open(settings_file, 'w') as fp:
                json.dump(projects, fp)
            dia.inform_echo("Saved directory '%s' as a project base." % path)
        else:
            dia.error("Error: directory '%s' is already saved as a project base." % path)
    else:
        dia.error("Error: directory '%s' does not exist." % path)


def do_remove_base(dia, settings_file, args):
    try:
        with open(settings_file, 'r') as fp:
            projects = json.load(fp)
    except (json.JSONDecodeError, FileNotFoundError):
        return

    path = os.path.realpath(args[0])

    if path in projects:
        projects.pop(path)
        with open(settings_file, 'w') as fp:
            json.dump(projects, fp)
        dia.inform_echo("Removed directory '%s' from project list." % path)
    else:
        dia.inform_echo("Error: directory '%s' is not a known project base." % path)


def find_tags(dia, matches, key_lang, order, ignored_tags, equivalent):
    ret = []
    key_lang = key_lang.lower()

    for line in matches.split(b'\n'):
        line = line.split(b'\t')
        if len(line) <= 3 or line[0] == b'!':
            continue
        entry = {}
        entry['name'] = line[0]
        for field in line[3:]:
            if len(field) == 1:
                entry['kind'] = field
            elif field.startswith(b'language:'):
                entry['lang'] = field[9:]

        contains = lambda a, x: all(map(lambda y: y in a, x))

        if contains(entry, ('lang', 'kind')):
            grp = entry['kind'].decode('ascii')
            grp = grp if equivalent is None else \
                  grp if grp not in equivalent else \
                  equivalent[grp]

            entry['kind'] = bytes(grp, 'ascii')
            entry_lang = entry['lang'].decode('ascii', errors='replace').lower()
            entry_name = entry['name'].decode('ascii', errors='replace')

            if (grp in order
                    and (key_lang == entry_lang
                            or (key_lang   in ('c', 'c\\+\\+') and
                                entry_lang in ('c', 'c++')))
                    and entry_name not in ignored_tags):
                ret.append(entry)
        else:
            dia.error("No lang/kind")

    return sorted(ret, key=lambda x: x['name'])


def strip_c(buf, dia):
    """This is the ugliest python function I've ever written and I'm ashamed
    that it exists. Can you tell that it's an almost line for line translation
    of a C program? The two embedded functions were macros.
    """
    pos = bytes(buf, 'ascii', errors='replace')
    single_q = double_q = slash = escape = skip = False
    space = 0
    buf = bytearray(len(pos) + 1)
    buf[0] = ord(b' ')
    i, x = 0, 1

    def check_quote(tocheck, other):
        nonlocal skip, escape
        if not other:
            if tocheck:
                if not escape:
                    tocheck = False
                    skip = True
            else:
                tocheck = True

        return tocheck, other

    def QUOTE():
        nonlocal double_q, single_q
        return double_q or single_q

    while i < len(pos):
        ch = chr(pos[i])
        if ch == '/':
            if not QUOTE():
                if slash:
                    x -= 1
                    end = i + pos[i:].find(b'\n')
                    if end < 0:
                        dia.error("Failed to find end of comment")
                        return
                    while pos[end - 1] == '\\':
                        end = pos[end+1:].find(b'\n')
                    i = end
                    if chr(buf[x-1]) == '\n':
                        skip = True
                else:
                    slash = True

        elif ch == '*':
            if not QUOTE() and slash:
                x -= 1
                end = i + pos[i:].find(b'*/')
                if end < 0:
                    dia.error("Failed to find end of comment")
                    return
                i = end + 2
                try:
                    ch = chr(pos[i])
                except IndexError:
                    break
                if ch == '\n' and chr(buf[x-1]) == '\n':
                    skip = True
                slash = False

        elif ch == '\n':
            if not escape:
                slash = double_q = False
                if (chr(buf[x-1]) == '\n'):
                    skip = True

        elif ch == '#':
            slash = False
            endln = i + pos[i+1:].find(b'\n')
            if chr(buf[x-1]) == '\n' and endln > 0:
                tmp = i + 1
                if chr(pos[i+1]).isspace():
                    while chr(pos[tmp]).isspace() and tmp < endln:
                        tmp += 1
                thing = bytes(pos[tmp:tmp + 7])
                if thing == b'include':
                    i = endln + 2
                    continue

        elif ch == '\\':
            pass

        elif ch == '"':
            double_q, single_q = check_quote(double_q, single_q)
            slash = False

        elif ch == "'":
            single_q, double_q = check_quote(single_q, double_q)
            slash = False

        else:
            slash = False

        escape = not escape if (ch == '\\') else False
        skip   = True       if (skip)       else (ch.isspace() and chr(buf[x-1]) == '\n')
        space  = space + 1  if (ch.isspace() and not skip) else 0

        if skip:
            skip = False
        elif not QUOTE() and space < 2:
            buf[x] = ord(ch)
            x += 1

        i += 1

    return bytes(buf[:x])


def tokenize(buf, dia):
    split = list(set(re.split(b'\W', buf)))
    return split


def bindex(a, x):
    """Locate the leftmost value exactly equal to x"""
    i = bisect.bisect_left(a, x)
    if i != len(a) and a[i] == x:
        return i
    return (-1)
