'''
Test parallel-vectorize with numpy.fromfunc.
Uses the work load from test_parallel_vectorize.
'''

from test_parallel_vectorize import *

import numpy as np

def main():
    module = Module.new(__name__)

    spufdef = SpecializedParallelUFunc(ParallelUFuncPosix(num_thread=2),
                                       UFuncCore_D_D(),
                                       Work_D_D())

    sppufunc = spufdef(module)

    module.verify()

    mpm = PassManager.new()
    pmbuilder = PassManagerBuilder.new()
    pmbuilder.opt_level = 3
    pmbuilder.populate(mpm)

    mpm.run(module)
#    print module

    # run

    exe = CExecutor(module)
    funcptr = exe.engine.get_pointer_to_function(sppufunc)
    print("Function pointer: %x" % funcptr)

    ptr_t = long # py2 only

    # Becareful that fromfunc does not provide full error checking yet.
    # If typenum is out-of-bound, we have nasty memory corruptions.
    # For instance, -1 for typenum will cause segfault.
    # If elements of type-list (2nd arg) is tuple instead,
    # there will also memory corruption. (Seems like code rewrite.)
    typenum = np.dtype(np.double).num
    ufunc = np.fromfunc([ptr_t(funcptr)], [[typenum, typenum]], 1, 1, [None])

    x = np.linspace(0., 10., 1000)
    x.dtype=np.double
#    print x
    ans = ufunc(x)
#    print ans

    if not ( ans == x/2.345 ).all():
        raise ValueError('Computation failed')
    else:
        print('Good')

if __name__ == '__main__':
    main()
