from parallel_vectorize import *
from llvm_cbuilder import shortnames as C
from llvm.core import *
import numpy as np
import unittest
from random import random

class TwoOne(CDefinition):

    def body(self, a, b):
        self.ret( (a * b).cast(self.OUT_TYPE) )

    @classmethod
    def specialize(cls, itype1, itype2, otype):
        cls._name_ = '.'.join(map(str, ['oneone', itype1, itype2, otype]))
        cls._retty_ = otype
        cls._argtys_ = [
            ('a', itype1),
            ('b', itype2),
        ]
        cls.OUT_TYPE = otype


class TestParallelVectorize(unittest.TestCase):
    def test_parallelvectorize_dd_d(self):
        self.template(C.double, C.double, C.double)

    def test_parallelvectorize_dd_f(self):
        self.template(C.double, C.double, C.float)

    def template(self, itype1, itype2, otype):
        module = Module.new(__name__)
        exe = CExecutor(module)

        def_twoone = TwoOne(itype1, itype2, otype)
        twoone = def_twoone(module)
        ufunc = parallel_vectorize_from_func(twoone, exe.engine)
        # print(module)
        module.verify()

        A = np.linspace(.0, 10., 1000)
        A.dtype = np.double
        B = np.linspace(-10., 0., 1000)
        B.dtype = np.double

        ans = ufunc(A, B)
        gold = A * B

        for x, y in zip(ans, gold):
            if y != 0:
                err = abs(x - y)/y
                self.assertLess(err, 1e-6)
            else:
                self.assertEqual(x, y)

if __name__ == '__main__':
    unittest.main()

