#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Mersenne Twister pseudo-random number generator

Provides the MersenneTwister class and mt_genrand function for generating
pseudo-random numbers using the 1999-10-28 integer variant of the Mersenne
Twister engine. Has methods for generating reals [0,1] [0,1) (0,1) intervals.

>>> Mt = mt19937.MersenneTwister(seed)
>>> return [Mt.genrand() for _ in range(10)]

Use MersenneTwister as a full implementation for generating multiple random
numbers from a single seeded state.

>>> return mt19937.mt_genrand(seed)

Use mt_genrand() for only generating the first random integer from state with a
specified seed.

See: <http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/VERSIONS/C-LANG/ver991029.html>
"""

__version__ = '1.0.0'
__date__    = '2020-08-24'
__author__  = 'Robert Jordan'
__credits__ = '''C implementation by Makoto Matsumoto and Takuji Nishimura - 1997, 1999
Converted to Python class and inlined mt_genrand by Robert Jordan - 2020
'''

__all__ = ['MersenneTwister', 'mt_genrand']

#######################################################################################
# --- original mt19937int.c ---
# 
# A C-program for MT19937: Integer version (1999/10/28) genrand() generates one
# pseudorandom unsigned integer (32bit) which is uniformly distributed among 0 to
# 2^32-1  for each call. sgenrand(seed) sets initial values to the working area
# of 624 words. Before genrand(), sgenrand(seed) must be called once. (seed is
# any 32-bit integer.)
# 
# Coded by Takuji Nishimura, considering the suggestions by Topher Cooper and
# Marc Rieffel in July-Aug. 1997.
# 
# This library is free software; you can redistribute it and/or modify it under
# the terms of the GNU Library General Public License as published by the Free
# Software Foundation; either version 2 of the License, or (at your option) any
# later version.
# 
# This library is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE. See the GNU Library General Public License for more
# details.
# 
# You should have received a copy of the GNU Library General Public License along
# with this library; if not, write to the Free Foundation, Inc., 59 Temple Place,
# Suite 330, Boston, MA  02111-1307, USA
# 
# Copyright (C) 1997, 1999 Makoto Matsumoto and Takuji Nishimura.
# Any feedback is very welcome. For any question, comments,
# see http://www.math.keio.ac.jp/matumoto/emt.html or email
# matumoto@math.keio.ac.jp
# 
# REFERENCE
# M. Matsumoto and T. Nishimura,
# "Mersenne Twister: A 623-Dimensionally Equidistributed Uniform
# Pseudo-Random Number Generator",
# ACM Transactions on Modeling and Computer Simulation,
# Vol. 8, No. 1, January 1998, pp 3--30.

#######################################################################################
# --- modifications ---
# 
# by Robert Jordan, 2020
# 
# * Port to Python 3, as OOP instance class
# * Add version number '1.0.0'
# * Rename file to mt19937.py (remove 'int')
#
# Updates:
# * Add genrand_real* methods
# * Remove lsgenrand() method
# * Rename sgenrand() method to seed()
# 
# Adds:
# * Add mt_genrand() inlined function for use with CatSystem2
#   (CS2 rarely uses more than one word generated from a single seed)
# * Add untemper() method for use with general reversing

#######################################################################################

from typing import List, NoReturn, Optional  # for hinting in declarations


## MT19937 PRNG CLASS ##

class MersenneTwister(object):
    """MersenneTwister() --> unseeded mt19937 object
    MersenneTwister(seed) --> seeded mt19937 object using seed

    Return a new MersenneTwister object for generating pseudo-random numbers
    from a single seeded state. Argument 'seed' should be a 32-bit unsigned
    integer.
    """
    __slots__ = ('_state', '_index')

    ### CONSTANTS ###

    #: size, period, and seed multiplier parameters for self._state array
    _N, _M, _F, = 624, 397, 69069
    #: constant vector MATRIX_A, used in self.twist()
    _MATRIX_A = 0x9908b0df
    #: tempering shifts and masks, used in MersenneTwister.temper() operation.
    _SHIFT_U, _SHIFT_S, _SHIFT_T, _SHIFT_L = 11, 7, 15, 18
    _MASK_B, _MASK_C = 0x9d2c5680, 0xefc60000
    #: default seed value, used in self.twist() when
    #: seed(integer) has not yet been called.
    INITIAL_SEED = 4357

    ### INSTANCE METHODS ###

    def __init__(self, seed:Optional[int]=None):
        self._state = [0] * self._N
        self._index = self._N+1
        if seed is not None:
            self.seed(seed)

    def seed(self, seed:int) -> NoReturn:
        """Seeds or reseeds the state array with a 32-bit unsigned integer.
        """
        if not isinstance(seed, int):
            raise TypeError('{0.__class__.__name__} seed() argument \'seed\' must be an integer, not {1.__class__.__name__}'.format(self, seed))
        elif not (0 <= seed <= 0xffffffff):
            raise ValueError('{0.__class__.__name__} seed() argument \'seed\' must be an 32-bit unsigned integer, not {1!r}'.format(self, seed))

        state, N, F = self._state, self._N, self._F  # (local for readability)
        self._index = N+1  # set to unseeded, in case of error
        for i in range(N):
            state[i]  = (seed & 0xffff0000)
            seed = (F * seed + 1) & 0xffffffff
            state[i] |= (seed & 0xffff0000) >> 16
            seed = (F * seed + 1) & 0xffffffff
        self._index = N  # set to seeded, untwisted

    def twist(self) -> NoReturn:
        """Twists the state array producing the next 624 random words. Called
        during genrand when the state array has run out of generated words.
        """
        # seed state if we haven't yet
        if self._index >= self._N+1:
            self.seed(self.INITIAL_SEED)  # 4357

        #UPPER_MASK, LOWER_MASK = 0x80000000, 0x7fffffff
        state, N, M = self._state, self._N, self._M  # (local for readability)
        mag01 = (0, self._MATRIX_A)
        y = 0
        for kk in range(  0, N-M):
            y = (state[kk] & 0x80000000) | (state[kk+1] & 0x7fffffff)
            state[kk] = state[kk+M] ^ (y >> 1) ^ mag01[y & 0x1]
        for kk in range(N-M, N-1):
            y = (state[kk] & 0x80000000) | (state[kk+1] & 0x7fffffff)
            state[kk] = state[kk-(N-M)] ^ (y >> 1) ^ mag01[y & 0x1]
        y = (state[N-1] & 0x80000000) | (state[0] & 0x7fffffff)
        state[N-1] ^= state[M-1] ^ (y >> 1) ^ mag01[y & 0x1]

        self._index = 0  # set to first word in the state

    def genrand(self) -> int:
        """Returns the next integer generated from the state, 32-bit unsigned.
        """
        # if we have no words left in the state or have not seeded
        if self._index >= self._N:
            self.twist()  # twist() will call seed() if unseeded
        y = self._state[self._index]
        self._index += 1
        return self.temper(y)
    
    def genrand_real1(self) -> float:
        """Returns the next real number generated from the state, [0,1] interval.
        """
        y = self.genrand()
        return (y * 2.3283064370807974e-10)

    def genrand_real2(self) -> float:
        """Returns the next real number generated from the state, [0,1) interval.
        """
        y = self.genrand()
        return (y * 2.3283064365386963e-10)
    
    def genrand_real3(self) -> float:
        """Returns the next real number generated from the state, (0,1) interval.
        """
        y = self.genrand()
        return ((y + 1.0) * 2.3283064359965952e-10)

    ### CLASS METHODS ###

    @classmethod
    def temper(cls, y:int) -> int:
        """Returns the tempered state value y, called during genrand.
        """
        y ^= (y >> cls._SHIFT_U)
        y ^= (y << cls._SHIFT_S) & cls._MASK_B
        y ^= (y << cls._SHIFT_T) & cls._MASK_C
        y ^= (y >> cls._SHIFT_L)
        return y & 0xffffffff

    @classmethod
    def untemper(cls, y:int) -> int:
        """Returns the un-tempered original state value of y. (for reversing)
        """
        y ^= (y >> cls._SHIFT_L)
        y ^= (y << cls._SHIFT_T) & cls._MASK_C
        for _ in range(7):
            y ^= (y << cls._SHIFT_S) & cls._MASK_B
        for _ in range(3):
            y ^= (y >> cls._SHIFT_U)
        return y & 0xffffffff



## OPTIMIZATION FUNCTIONS (optional) ##

## Uncomment this if removing inlined implementation
# def mt_genrand(seed:int) -> int:
#     """mt_genrand(seed) -> integer
# 
#     Returns the first integer generated from the state with a specified seed.
#     This is an optimized function for when only the first integer is ever
#     needed from the state.
# 
#     This is an alias for MersenneTwister(seed).genrand() -> integer
#     """
#     return MersenneTwister(seed).genrand()

def mt_genrand(seed:int) -> int:
    """mt_genrand(seed) -> integer

    Returns the first integer generated from the state with a specified seed.
    This is an optimized function for when only the first integer is ever
    needed from the state.

    This is an alias for MersenneTwister(seed).genrand() -> integer

    See: <https://gist.github.com/trigger-segfault/5a2dad647c782fbab11bda2736c08abc>
    """
    if not isinstance(seed, int):
        raise TypeError('mt_genrand() argument \'seed\' must be an integer, not {0.__class__.__name__}'.format(seed))
    elif not (0 <= seed <= 0xffffffff):
        raise ValueError('mt_genrand() argument \'seed\' must be a 32-bit unsigned integer, not {0!r}'.format(seed))
    cls = MersenneTwister
    #UPPER_MASK, LOWER_MASK = 0x80000000, 0x7fffffff
    M, F = cls._M, cls._F  # (local for readability)
    mag01 = (0, cls._MATRIX_A)

    # generate the required words needed from the state array
    # and operate on/store them via the output, y
    # state[0]
    y  = (seed & 0x80000000)  # state[0], 0xffff0000 & UPPER_MASK
    seed = (F * seed + 1) & 0xffffffff
    seed = (F * seed + 1) & 0xffffffff
    # state[1]
    y |= (seed & 0x7fff0000)  # state[1], 0xffff0000 & LOWER_MASK
    seed = (F * seed + 1) & 0xffffffff
    y |= (seed & 0xffff0000) >> 16
    seed = (F * seed + 1) & 0xffffffff

    # state[M] (partial)
    y = (y >> 1) ^ mag01[y & 0x1]
    for i in range(2, M):  # get to state[M]
        seed = (F * seed + 1) & 0xffffffff
        seed = (F * seed + 1) & 0xffffffff
    # state[M] (final)
    y ^= (seed & 0xffff0000)  # state[M]
    seed = (F * seed + 1)  ##& 0xffffffff
    y ^= (seed & 0xffff0000) >> 16

    return cls.temper(y)


## CLEANUP ##

del List, NoReturn, Optional  # only used during declarations
