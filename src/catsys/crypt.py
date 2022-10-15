#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""CatSystem encryption utilities

Provides the Blowfish cipher and MersenneTwister PRNG classes, functions for
V_CODE extraction, and data encryption used by CatSystem
"""

__version__ = '1.0.1'
__date__    = '2022-02-15'
__author__  = 'Robert Jordan'

__all__ = ['MersenneTwister', 'mt_genrand', 'Blowfish', 'read_cs2codes', 'vcode_seed', 'beaufort_key', 'beaufort_cipher', 'beaufort_decipher', 'beaufort_encipher', 'keyfile_generate']

#######################################################################################

import collections, hashlib, os, struct
from typing import List, NoReturn, Optional, Tuple, Type, Union  # for hinting in declarations

# local imports
from .blowfish import Blowfish
from .mt19937 import MersenneTwister, mt_genrand


## INTERNAL CONSTANTS ##

#CS2_ENCODING:str = 'shift_jis'
CS2_ENCODING:str = 'cp932'
CS2_DEBUG_KEY_CONST:bytes = b'@@--cs2-debug-key--@@'
CS2_COMMON_KEY_CONST:bytes = b'@@--cs2-common-key--@@'
CS2_OPEN_VCODE:bytes = b'open_cs2'
# USER CONSTANTS

KEY_ACTIVATE:str = 'activate'  # Unknown
KEY_DIRECT:str   = 'direct'    # Validate non-CD installs, works anywhere
KEY_KEY:str      = 'key'       # Validate CD installs on a per-computer basis
KEY_KEYCOM:str   = 'key_com'   # Common key, used when key.dat is invalid/not found
KEY_DEBUG:str    = 'cs2_debug_key'  # Enable debug mode
KEY_GK:str       = 'cs2_gk'    # Unknown, a much older key type
KEY_KIFDUMMY:str = '__key__'   # Dummy entry for encryption in KIF (.int) archives


## NAMED TUPLES ##

# decrypted Cs2 KEY_CODE, V_CODE, and V_CODE2
#NOTE: (V_CODE2 does not exist in earier games)
cs2codes = collections.namedtuple('cs2codes', ('keycode', 'vcode1', 'vcode2'))
# single decrypted cs2_gk.dat entry
globalentry = collections.namedtuple('globalentry', ('name', 'md5hash'))


## PREDECLARE TYPES ##

PE = Type['pefile.PE']


#######################################################################################

#region  ## HELPER FUNCTIONS ##

## FIND RESOURCES ##

def find_resource(pe:PE, typename:str, name:str) -> bytes:
    directory = getattr(pe, 'DIRECTORY_ENTRY_RESOURCE', None)
    if not directory: # is None: #or not directory:
        pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_RESOURCE']])
    
    for entry_t in directory.entries:
        #print(entry_t.name.decode() if entry_t.name else entry_t.id)
        if not entry_t.name or entry_t.name.decode() != typename:
            continue
        for entry_n in entry_t.directory.entries:
            #print(entry_n.name.decode() if entry_n.name else entry_n.id)
            if not entry_n.name or entry_n.name.decode() != name:
                continue
            for entry_l in entry_n.directory.entries:
                resstruct = entry_l.data.struct
                #return resstruct
                return pe.get_data(resstruct.OffsetToData, resstruct.Size)
    return None


## HELPER FUNCTIONS ##

def _get_bytes(s:Union[str,bytes]) -> bytes:
    return s.encode(CS2_ENCODING) if isinstance(s, str) else s


def cstring(data:bytes, encoding:str=None, *, terminator:Union[int,bytes]=0):
    term = data.find(terminator) # find null terminator
    text = data[:term if term != -1 else len(data)]
    return text if encoding is None else text.decode(encoding)

def cstring_from(data:bytes, offset:int=0, count:int=None, encoding:str=None, *, terminator:Union[int,bytes]=0):
    if count is None: count = len(data) - offset
    start, end = offset, min(len(data), offset + count)
    
    term = data.find(terminator, start, end)
    text = data[start:term if term != -1 else end]
    return text if encoding is None else text.decode(encoding)

#endregion


#######################################################################################

#region ## CS2CODES ##

def keycode_crypt(keycode:Union[bytes,str]) -> bytes:
    """keycode_crypt(keycode) -> bytes
    keycode_crypt(keycode) -> bytes
    """
    if isinstance(keycode, str):
        keycode = keycode.encode(CS2_ENCODING)
    elif not isinstance(keycode, (bytes,bytearray)):
        raise TypeError('keycode_crypt() argument \'keycode\' must be a str or bytes-likes object, not {0.__class__.__name__}'.format(keycode))
    return bytes(b ^ 0xcd for b in keycode)

def get_blowfish(key:Union[str,bytes,Blowfish]) -> Blowfish:
    """get_blowfish(key) -> blowfish
    get_blowfish(blowfish) -> blowfish
    """
    if isinstance(key, (str,bytes,bytearray)):
        if isinstance(key, str):
            key = key.encode(CS2_ENCODING)
        return Blowfish(key)
    elif hasattr(key, 'decrypt'):
        return key
    else:
        raise TypeError('get_blowfish() argument \'key\' must be a str or bytes-likes object, or Blowfish cipher, not {0.__class__.__name__}'.format(key))


def vcode_encrypt(vcode:Union[bytes,str], key:Union[bytes,str,Blowfish]) -> bytes:
    """vcode_encrypt(vcode, key) -> bytes
    vcode_encrypt(vcode, blowfish) -> bytes
    """
    if isinstance(vcode, str):
        vcode = vcode.encode(CS2_ENCODING)
    elif not isinstance(vcode, (bytes,bytearray)):
        raise TypeError('vcode_encrypt() argument \'vcode\' must be a str or bytes-likes object, not {0.__class__.__name__}'.format(vcode))
    if isinstance(key, (str,bytes,bytearray)):
        if isinstance(key, str):
            key = key.encode(CS2_ENCODING)
        key = Blowfish(key)
    elif not hasattr(key, 'decrypt'):
        raise TypeError('vcode_encrypt() argument \'key\' must be a str or bytes-likes object, or Blowfish cipher, not {0.__class__.__name__}'.format(key))
    bf = key
    # pad length to multiple of 8
    vcode = vcode.ljust((len(vcode) + 7) & ~0x7, b'\x00')
    # encrypt vcode
    return bf.encrypt(vcode)

def vcode_decrypt(vcode:Union[bytes,str], key:Union[bytes,str,Blowfish]) -> bytes:
    """vcode_decrypt(vcode, key) -> bytes
    vcode_decrypt(vcode, blowfish) -> bytes
    """
    if isinstance(vcode, str):
        vcode = vcode.encode(CS2_ENCODING)
    elif not isinstance(vcode, (bytes,bytearray)):
        raise TypeError('vcode_decrypt() argument \'vcode\' must be a str or bytes-likes object, not {0.__class__.__name__}'.format(vcode))
    if isinstance(key, (str,bytes,bytearray)):
        if isinstance(key, str):
            key = key.encode(CS2_ENCODING)
        key = Blowfish(key)
    elif not hasattr(key, 'decrypt'):
        raise TypeError('vcode_decrypt() argument \'key\' must be a str or bytes-likes object, or Blowfish cipher, not {0.__class__.__name__}'.format(key))
    bf = key
    # pad length to multiple of 8
    vcode = vcode.ljust((len(vcode) + 7) & ~0x7, b'\x00')
    # decrypt vcode and trim null bytes
    return bf.decrypt(vcode).rstrip(b'\x00')

def read_cs2codes(filename:str, defaultkey:bytes=b'windmill') -> Tuple[bytes, bytes, bytes]:
    """read_cs2codes(filename, defaultkey=b'windmill') -> keycode, vcode1, vcode2
    """
    import pefile
    pe = pefile.PE(filename, fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY['IMAGE_DIRECTORY_ENTRY_RESOURCE']])
    try:
        key_code:bytes = find_resource(pe, 'KEY_CODE', 'KEY')
        v_code1:bytes  = find_resource(pe, 'V_CODE',  'DATA')
        v_code2:bytes  = find_resource(pe, 'V_CODE2', 'DATA')
    finally:  
        pe.close()
    # Default KEY_CODE (at least with older versions), is 'windmill'
    if (key_code or defaultkey) and (v_code1 or v_code2):
        isdefault = not key_code
        key_code = keycode_crypt(key_code) if key_code else defaultkey
        bf = Blowfish(key_code)
        v_code1 = vcode_decrypt(v_code1, bf) if v_code1 else None
        v_code2 = vcode_decrypt(v_code2, bf) if v_code2 else None
        return cs2codes([key_code] if isdefault else key_code, v_code1, v_code2)
    return cs2codes(None, None, None)


## VCODE SEED ##

#def vcode_seed(vcode:Union[bytes,str], init:Optional[int]=None) -> int:
def vcode_seed(vcode:Union[bytes,str]) -> int:
    """vcode_seed(vcode) -> int
    """
    if isinstance(vcode, str):
        vcode = vcode.encode(CS2_ENCODING)
    elif not isinstance(vcode, (bytes,bytearray)):
        raise TypeError('vcode_seed() argument \'vcode\' must be a str or bytes-likes object, not {0.__class__.__name__}'.format(vcode))
    # if init is not None:
    #     if not isinstance(init, int):
    #         raise TypeError('vcode_seed() argument \'init\' must be an int, not {0.__class__.__name__}'.format(init))
    #     elif not (0 <= init <= 0xffffffff):
    #         raise ValueError('vcode_seed() argument \'init\' must be a 32-bit unsigned integer, not {0!r}'.format(init))
    # crc = 0xffffffff if init is None else init
    crc = 0xffffffff  # init
    for o in vcode:
        if o == 0: # break on null termination
            break
        # get next octet from vcode data
        crc ^= (o << 24)
        for _ in range(8):
            if crc & 0x80000000:  # check MSB
                crc = (crc << 1) ^ 0x04c11db7  # polynomial
            else:
                crc <<= 1
        # apply xorout after every octet
        crc = ~crc & 0xffffffff  # xorout
    return crc

# def mt_genrand(seed:int) -> int:
#     return MersenneTwister(seed).genrand()

#endregion


#######################################################################################

#region ## BEAUFORT CIPHER ##

def beaufort_key(tocseed:int, index:int) -> int:
    """beaufort_key(tocseed, index) -> int
    """
    mtkey = mt_genrand((tocseed + index) & 0xffffffff)
    cipherkey = ((mtkey >> 24) + (mtkey >> 16) + (mtkey >> 8) + mtkey) & 0xff
    return cipherkey

def beaufort_cipher(buffer:Union[bytes,str], key:int) -> bytes:
    if isinstance(buffer, str):
        buffer = buffer.encode(CS2_ENCODING)
    elif not isinstance(buffer, (bytes,bytearray)):
        raise TypeError('beaufort_cipher() argument \'buffer\' must be a str or bytes-likes object, not {0.__class__.__name__}'.format(buffer))
    if not isinstance(key, int):
        raise TypeError('beaufort_cipher() argument \'key\' must be an integer, not {0.__class__.__name__}'.format(key))
    # Class Constants
    LENGTH, HALF = 52, 26
    key %= LENGTH
    result:bytearray = bytearray(len(buffer))
    for i, c in enumerate(buffer):
        if c == 0:
            break  # break on null-termination
        if (c >= ord('A') and c <= ord('Z')) or (c >= ord('a') and c <= ord('z')):
            # Get index of char in the cipher, the smart way
            # Upper (A=0,  Z=25)  Lower (a=26, z=51)
            idx:int = c - (ord('A') if c <= ord('Z') else (ord('a')-HALF))
            # Add key + c "state"
            idx = (idx + key) % LENGTH
            # Reverse the index and assign the char
            # Lower (25=a,  0=z)  Upper (51=A, 26=Z)
            c = (ord('z') if idx < HALF else (ord('Z')+HALF)) - idx
        result[i] = c
        key += 1
    return bytes(result)

def beaufort_decipher(buffer:bytes, key:int) -> str:
    s:str = beaufort_cipher(buffer, key)
    return s.rstrip(b'\x00').decode(CS2_ENCODING)

def beaufort_encipher(s:str, key:int, size:int) -> bytes:
    buffer:bytes = s.encode(CS2_ENCODING).ljust(size, b'\x00')
    return beaufort_cipher(buffer, key)

#endregion


#######################################################################################

#region ## KEY FILES ##

## KEY FILE HELPERS ##

def get_windowsroot() -> str:
    """get_windowsroot() -> str
    
    Requires calling on user's machine (for keyfiles)
    """
    windowsPath = os.path.expandvars('%WINDIR%')
    return os.path.splitdrive(windowsPath)[0] + '\\'

# def get_volumeserialnum(volumepath:Union[str,bytes]=...) -> int:
def get_volumeserialnum(volumepath:Union[str,bytes]) -> int:
    """get_volumeserialnum(volumepath) -> int
    
    Requires ctypes, WINAPI, and calling on user's machine (for keyfiles)
    """
    from ctypes import byref, WinDLL
    from ctypes.wintypes import DWORD
    # if volumepath is Ellipsis:
    #     volumepath = get_windowsroot()
    if _kernel32 is None:
        _kernel32 = WinDLL('kernel32', use_last_error=True)
    volserialnum = DWORD(0)
    maxcomplen = DWORD(0)
    filesysflags = DWORD(0)
    if isinstance(volumepath, bytes):
        GetVolumeInformation = _kernel32.GetVolumeInformationA
    elif isinstance(volumepath, str):
        GetVolumeInformation = _kernel32.GetVolumeInformationW
    else:
        raise TypeError('get_volumeserialnum() argument \'volumepath\' must be bytes or str object, not {0.__class__.__name__}'.format(volumepath))
    res = GetVolumeInformation(
        volumepath, # <in>
        None, DWORD(0),  # (volumeNameBuf, volumeNameSize)
        byref(volserialnum), # <out>
        byref(maxcomplen),   # (unused)
        byref(filesysflags), # (unused)
        None, DWORD(0))  # (fileSysNameBuf, fileSysNameSize)
    if not res:
        raise WindowsError('get_volumeserialnum() error argument \'volumepath\' may be invalid, {0!r}'.format(volumepath))
    return volserialnum.value


## KEY FILE ##

def keyfile_seed(vcode1:Union[str,bytes], keytype:str, volserialnum:int=...) -> int:
    """keyfile_seed(vcode1, keytype) -> int
    keyfile_seed(vcode1, KEY_KEY[, volserialnum]) -> int

    vcode1 (V_CODE) is used for keyfile generation, not vcode2 (V_CODE2)
    """
    # ensure we're operating on bytes, do this ahead
    # of time for the KEY_DEBUG string concatenation
    if isinstance(vcode1, str): 
        vcode1 = vcode1.encode(CS2_ENCODING)
    
    if keytype == KEY_DIRECT:   # direct installs (valid anywhere)
        return vcode_seed(vcode1)

    elif keytype == KEY_KEY:    # cd installs (local to computer)
        if volserialnum is Ellipsis:
            # get_volumeserialnum requires ctypes, WINAPI, and calling on user's machine
            volserialnum = get_volumeserialnum(get_windowsroot())
        return (vcode_seed(vcode1) + volserialnum) & 0xffffffff

    elif keytype == KEY_KEYCOM:  # common key, used in-place of key.dat when missing (older)
        return vcode_seed(vcode1 + CS2_COMMON_KEY_CONST) #b'@@--cs2-common-key--@@')

    elif keytype == KEY_DEBUG:  # debug mode
        return vcode_seed(vcode1 + CS2_DEBUG_KEY_CONST) #b'@@--cs2-debug-key--@@')

    elif keytype == KEY_ACTIVATE:  # unsupported (possibly plaintext like cdid.id)
        pass
    elif keytype == KEY_GK:  # unsupported (older key format, not yet encountered in assembly)
        ## FORMAT:
        ## [32s] cs2 executable name
        ## [16s] unk hash
        ## [32s] installer executable name
        ## [16s] unk hash
        pass
    elif keytype == KEY_KIFDUMMY:  # not a real keytype, just a dummy
        pass
    else:
        raise ValueError('keyfile_seed() keytype {0!r} is not a valid keytype'.format(keytype))
    raise ValueError('keyfile_seed() keytype {0!r} is unsupported'.format(keytype))


def keyfile_generate(keyseed:int, keytype:str=...) -> bytes:
    """keyfile_generate(keyseed) -> bytes
    keyfile_generate(vcode1, keytype) -> bytes

    vcode1 (V_CODE) is used for keyfile generation, not vcode2 (V_CODE2)
    """
    if isinstance(keyseed, (str,bytes,bytearray)):
        if keytype is Ellipsis:
            raise TypeError('keyfile_generate() from vcode: argument \'keytype\' is required')
        elif not isinstance(keytype, str):
            raise TypeError('keyfile_generate() from vcode: argument \'keytype\' must be a str object, not {0.__class__.__name__}'.format(keytype))
        vcode1 = keyseed
        keyseed = keyfile_seed(vcode1, keytype)
    elif not isinstance(keyseed, int):
        raise TypeError('keyfile_generate() first argument must be an int, str, or bytes-like object, not {0.__class__.__name__}'.format(keyseed))
    
    # generate PRNG key then data
    mt = MersenneTwister(keyseed)
    filekey  = struct.pack('<16I', *[mt.genrand() for _ in range(16)])
    filedata = struct.pack('<16I', *[mt.genrand() for _ in range(16)])
    # now encrypt generated data
    bf = Blowfish(filekey)
    return bf.encrypt(filedata)

#endregion


#######################################################################################

#region ## GLOBAL KEY ##

def globalkey_generate(vcode1seed:int, *filenames:Union[str,Tuple[str,str],globalentry], version2:bool=False) -> bytes:
    """globalkey_generate(vcode1seed, *filenames) -> bytes
    globalkey_generate(vcode1, *filenames) -> bytes
    globalkey_generate(vcode1, *(filename, name_in_key), ...) -> bytes
    globalkey_generate(vcode1, *globalentries) -> bytes

    vcode1 (V_CODE) is used for global key (cs2_gk.dat) generation, not vcode2 (V_CODE2)
    vcode2 (V_CODE2) likely didn't exist back when "cs2_gk.dat" was used
        (this is a very old key type)

    version2 - specifies 64-byte filename entries, seen with newer engines (but key usage itself has never been seen)
    """
    import hashlib

    if isinstance(vcode1seed, (str,bytes,bytearray)):
        vcode1 = vcode1seed
        vcode1seed = vcode_seed(vcode1)
    elif not isinstance(vcode1seed, int):
        raise TypeError('globalkey_generate() first argument must be an int, str, or bytes-like object, not {0.__class__.__name__}'.format(vcode1seed))
    
    # color me shocked... absolutely ZERO Mersenne Twister being used!
    bf = Blowfish(struct.pack('<I', vcode1seed))

    gkey = bytearray() # key file with a variable number of file entries
    for filename in filenames:
        globalkey = None
        if isinstance(filename, globalentry):
            globalkey = filename
            #globalentryname, md5hash = globalentry.name, globalentry.md5hash
        elif isinstance(filename, tuple):
            filename, name_in_key = filename
            globalkey = globalkey_fromfile(filename, name=name_in_key)
        else:
            globalkey = globalkey_fromfile(filename)
            
        # Entry is 32-byte Shift JIS filename and 16-byte encrypted MD5 hash
        # MD5 hash of file, encrypted with Blowfish, using V_CODE1 seed as key
        # with open(filename, 'rb') as f:
        #     md5hash = hashlib.md5(f.read()).digest() # read file and generate MD5 hash
        key = bf.encrypt(globalkey.md5hash) # encrypt MD5 hash
        #TODO: file names can be paths, relative or full, but they won't fit
        #      in the 32/64-byte name field, relative paths SHOULD be supported!
        #name = os.path.basename(filename).encode(CS2_ENCODING) # just the filename, no path(?)
        name = globalkey.name.encode(CS2_ENCODING) # just the filename, no path(?)
        if version2:
            gkey.extend(struct.pack('<64s 16s', name, key))
        else:
            gkey.extend(struct.pack('<32s 16s', name, key))
    
    return gkey

def globalkey_fromfile(filename:str, name:Optional[str]=None) -> globalentry:
    """globalkey_fromfile(filename) -> globalentry(basename, md5hash)
    globalkey_fromfile(filename, name) -> globalentry(name, md5hash)
    """
    import hashlib

    if name is None:
        name = os.path.basename(filename)
    elif not isinstance(name, str):
        raise TypeError('globalkey_fromfile() \'name\' argument must be a str or None, not {0.__class__.__name__}'.format(name))

    with open(filename, 'rb') as f:
        md5hash = hashlib.md5(f.read()).digest() # read file and generate MD5 hash
    return globalentry(name, md5hash)

def globalkey_build(*filenames:Union[str,Tuple[str,str]]) -> List[globalentry]:
    """globalkey_build(*filenames) -> [globalentry, ...]
    globalkey_build(*(filename, name_in_key), ...) -> [globalentry, ...]
    """
    globalkeys = []
    for filename in filenames:
        if isinstance(filename, tuple):
            filename, name_in_key = filename
            globalkeys.append(globalkey_fromfile(filename, name=name_in_key))
        else:
            globalkeys.append(globalkey_fromfile(filename))
    
    return globalkeys

def globalkey_read(vcode1seed:int, file, *, version2:bool=False) -> List[globalentry]:
    """globalkey_read(vcode1seed, "cs2_gk.dat") -> [globalentry, ...]
    globalkey_read(vcode1, "cs2_gk.dat") -> [globalentry, ...]

    Returns filenames and decrypted MD5 hashes stored in a global key file (cs2_gk.dat)

    version2 - specifies 64-byte filename entries, seen with newer engines (but key usage itself has never been seen)
    """
    import hashlib
    
    if isinstance(file, str):
        with open(file, 'rb') as f:
            return globalkey_read(vcode1seed, f)
    
    if isinstance(vcode1seed, (str,bytes,bytearray)):
        vcode1 = vcode1seed
        vcode1seed = vcode_seed(vcode1)
    elif not isinstance(vcode1seed, int):
        raise TypeError('globalkey_read() first argument must be an int, str, or bytes-like object, not {0.__class__.__name__}'.format(vcode1seed))
    
    # color me shocked... absolutely ZERO Mersenne Twister being used!
    bf = Blowfish(struct.pack('<I', vcode1seed))

    file.seek(0, 2)
    length = file.tell()
    file.seek(0, 0)
    
    globalkeys = []
    for i in range(length // 0x30):
        if version2:
            name, key = struct.unpack('<64s 16s', file.read(0x50))
        else:
            name, key = struct.unpack('<32s 16s', file.read(0x30))
        name = name.rstrip(b'\x00').decode(CS2_ENCODING)
        md5hash = bf.decrypt(key)
        globalkeys.append(globalentry(name, md5hash))
    
    return globalkeys

#endregion


#######################################################################################

## CLEANUP ##

del List, NoReturn, Optional, Tuple, Type, Union  # only used during declarations

