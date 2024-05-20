# Released under the MIT License. See LICENSE for details.
#
"""Misc util calls/etc.

Ideally the stuff in here should migrate to more descriptive module names.
"""

from __future__ import annotations

from typing import TYPE_CHECKING, overload


if TYPE_CHECKING:
    from typing import Sequence, Literal
    from pathlib import Path


def explicit_bool(value: bool) -> bool:
    """Simply return input value; can avoid unreachable-code type warnings."""
    return value


def extract_flag(args: list[str], name: str) -> bool:
    """Given a list of args and a flag name, returns whether it is present.

    The arg flag, if present, is removed from the arg list.
    """
    from efro.error import CleanError

    count = args.count(name)
    if count > 1:
        raise CleanError(f'Flag {name} passed multiple times.')
    if not count:
        return False
    args.remove(name)
    return True


@overload
def extract_arg(
    args: list[str], name: str, required: Literal[False] = False
) -> str | None: ...


@overload
def extract_arg(args: list[str], name: str, required: Literal[True]) -> str: ...


def extract_arg(
    args: list[str], name: str, required: bool = False
) -> str | None:
    """Given a list of args and an arg name, returns a value.

    The arg flag and value are removed from the arg list.
    raises CleanErrors on any problems.
    """
    from efro.error import CleanError

    count = args.count(name)
    if not count:
        if required:
            raise CleanError(f'Required argument {name} not passed.')
        return None

    if count > 1:
        raise CleanError(f'Arg {name} passed multiple times.')

    argindex = args.index(name)
    if argindex + 1 >= len(args):
        raise CleanError(f'No value passed after {name} arg.')

    val = args[argindex + 1]
    del args[argindex : argindex + 2]

    return val


def replace_section(
    text: str,
    begin_marker: str,
    end_marker: str,
    replace_text: str = '',
    keep_markers: bool = False,
    error_if_missing: bool = True,
) -> str:
    """Replace all text between two marker strings (including the markers)."""
    if begin_marker not in text:
        if error_if_missing:
            raise RuntimeError(f"Marker not found in text: '{begin_marker}'.")
        return text
    splits = text.split(begin_marker)
    if len(splits) != 2:
        raise RuntimeError(
            f"Expected one marker '{begin_marker}'"
            f'; found {text.count(begin_marker)}.'
        )
    before_begin, after_begin = splits
    splits = after_begin.split(end_marker)
    if len(splits) != 2:
        raise RuntimeError(
            f"Expected one marker '{end_marker}'"
            f'; found {text.count(end_marker)}.'
        )
    _before_end, after_end = splits
    if keep_markers:
        replace_text = f'{begin_marker}{replace_text}{end_marker}'
    return f'{before_begin}{replace_text}{after_end}'


def readfile(path: str | Path) -> str:
    """Read a utf-8 text file into a string."""
    with open(path, encoding='utf-8') as infile:
        return infile.read()


def writefile(path: str | Path, txt: str) -> None:
    """Write a string to a utf-8 text file."""
    with open(path, 'w', encoding='utf-8') as outfile:
        outfile.write(txt)


def replace_exact(
    opstr: str, old: str, new: str, count: int = 1, label: str | None = None
) -> str:
    """Replace text ensuring that exactly x occurrences are replaced.

    Useful when filtering data in some predefined way to ensure the original
    has not changed.
    """
    found = opstr.count(old)
    label_str = f' in {label}' if label is not None else ''
    if found != count:
        raise RuntimeError(
            f'Expected {count} string occurrence(s){label_str};'
            f' found {found}. String: {repr(old)}'
        )
    return opstr.replace(old, new)


def get_files_hash(
    filenames: Sequence[str | Path],
    extrahash: str = '',
    int_only: bool = False,
    hashtype: Literal['md5', 'sha256'] = 'md5',
) -> str:
    """Return a hash for the given files."""
    import hashlib

    if not isinstance(filenames, list):
        raise RuntimeError(f'Expected a list; got a {type(filenames)}.')
    if TYPE_CHECKING:
        # Help Mypy infer the right type for this.
        hashobj = hashlib.md5()
    else:
        hashobj = getattr(hashlib, hashtype)()
    for fname in filenames:
        with open(fname, 'rb') as infile:
            while True:
                data = infile.read(2**20)
                if not data:
                    break
                hashobj.update(data)
    hashobj.update(extrahash.encode())

    if int_only:
        return str(int.from_bytes(hashobj.digest(), byteorder='big'))

    return hashobj.hexdigest()


def get_string_hash(
    value: str,
    int_only: bool = False,
    hashtype: Literal['md5', 'sha256'] = 'md5',
) -> str:
    """Return a hash for the given files."""
    import hashlib

    if not isinstance(value, str):
        raise TypeError('Expected a str.')
    if TYPE_CHECKING:
        # Help Mypy infer the right type for this.
        hashobj = hashlib.md5()
    else:
        hashobj = getattr(hashlib, hashtype)()
    hashobj.update(value.encode())

    if int_only:
        return str(int.from_bytes(hashobj.digest(), byteorder='big'))

    return hashobj.hexdigest()
