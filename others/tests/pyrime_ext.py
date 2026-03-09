"""
pyrime_ext — monkey-patches pyrime.rime with functions missing from its
Cython bindings, and provides convenience helpers.

Usage:
    import pyrime_ext          # patches pyrime.rime in-place
    from pyrime import rime    # now has set_option, type_input, is_operational

Call pyrime_ext.init() instead of rime.init() so the patch is installed
after librime is fully initialised.
"""

import ctypes
import os

from pyrime import rime

# set_option is the 28th function pointer in the RimeApi struct.
# Byte offset = 8 (int data_size + 4-byte alignment padding)
#             + 27 × 8 (preceding function pointers on 64-bit)
#             = 224
_SET_OPTION_OFFSET = 224


def init(shared_data_dir="/usr/share/rime-data", user_data_dir=None, **kwargs):
    """Initialise rime and patch set_option into pyrime.rime."""
    if user_data_dir is None:
        user_data_dir = os.path.join(os.path.expanduser("~"), ".config", "ibus", "rime")

    rime.init(shared_data_dir=shared_data_dir, user_data_dir=user_data_dir, **kwargs)

    lib = ctypes.CDLL("librime.so.1")
    lib.rime_get_api.restype = ctypes.c_void_p
    api = lib.rime_get_api()
    fp = ctypes.c_void_p.from_address(api + _SET_OPTION_OFFSET).value
    _raw = ctypes.CFUNCTYPE(None, ctypes.c_size_t, ctypes.c_char_p, ctypes.c_int)(fp)

    def set_option(session_id, option, value):
        _raw(session_id,
             option.encode() if isinstance(option, str) else option,
             int(value))

    rime.set_option = set_option


def type_input(sid, text):
    """Feed text into a session character by character via process_key."""
    rime.clear_composition(sid)
    for ch in text:
        rime.process_key(sid, ord(ch), 0)


def is_operational(sid):
    """Return True if the session's schema prism is compiled and produces candidates."""
    rime.clear_composition(sid)
    rime.process_key(sid, ord("x"), 0)
    ctx = rime.get_context(sid)
    result = ctx.menu.num_candidates > 0
    rime.clear_composition(sid)
    return result
