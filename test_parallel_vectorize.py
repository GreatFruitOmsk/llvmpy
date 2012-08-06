from parallel_vectorize import *


class Work_D_D(CDefinition):
    _name_ = 'copy_d_d'
    _retty_ = C.double
    _argtys_ = [
        ('inval', C.double),
    ]
    def body(self, inval):
        self.ret(inval / self.constant(inval.type, 2.345))

class UFuncCore_D_D(UFuncCore):
    '''
    Specialize UFuncCore for double input, double output.
    '''
    def _do_work(self, common, item, tid):
        ufunc_type = Type.function(C.double, [C.double])
        ufunc_ptr = CFunc(self, common.func.cast(C.pointer(ufunc_type)).value)

        inbase = common.args[0]
        outbase = common.args[1]

        instep = common.steps[0]
        outstep = common.steps[0]

        indata = inbase[item * instep].reference().cast(C.pointer(C.double))
        outdata = outbase[item * outstep].reference().cast(C.pointer(C.double))

        res = ufunc_ptr(indata.load())
        outdata.store(res)


class Tester(CDefinition):
    '''
    Generate test.
    '''
    _name_ = 'tester'

    def body(self):
        # depends
        module = self.function.module

        ThreadCount = 2
        ArgCount = 2
        WorkCount = 10000

        parallel_ufunc = CFunc(self, ParallelUFuncPosix.define(module,
                               ThreadCount=ThreadCount))
        ufunc_core = CFunc(self, UFuncCore_D_D.define(module))
        worker = CFunc(self, Work_D_D.define(module))

        # real work
        NULL = self.constant_null(C.void_p)

        args = self.array(C.char_p, 2, name='args')

        args_double = []
        for t in range(ThreadCount):
            args_for_thread = self.array(C.double, WorkCount)
            args[t].assign(args_for_thread.cast(C.char_p))
            args_double.append(args_for_thread)

        dims = self.array(C.intp, 1, name='dims')
        dims[0].assign(self.constant(C.intp, WorkCount))

        steps = self.array(C.intp, ArgCount, name='steps')

        for c in range(ArgCount):
            steps[c].assign(self.constant(C.intp, 8))

        # populate data
        inbase = args_double[0]

        i = self.var(C.intp, 0)
        with self.loop() as loop:
            with loop.condition() as setcond:
                setcond( i < self.constant(C.intp, WorkCount) )
            with loop.body():
                inbase[i].assign(i.cast(C.double))
                i += self.constant(C.intp, 1)

        # call parallel ufunc
        parallel_ufunc(worker.cast(C.void_p), ufunc_core.cast(C.void_p),  args,
                       dims, steps, NULL)

        # check error
        outbase = args_double[-1]
        with self.for_range(self.constant(C.intp, WorkCount)) as (loop, i):
            test = outbase[i] != (inbase[i] / self.constant(C.double, 2.345))
            with self.ifelse( test ) as ifelse:
                with ifelse.then():
                    self.debug("Invalid data at i =", i, outbase[i], inbase[i])

        self.ret()

def main():
    NUM_THREAD = 2
    module = Module.new(__name__)

    mpm = PassManager.new()
    pmbuilder = PassManagerBuilder.new()
    pmbuilder.opt_level = 3
    pmbuilder.populate(mpm)

    fntester = Tester.define(module)

#    print(module)
    module.verify()

    mpm.run(module)

    print('optimized'.center(80,'-'))
    print(module)

    # run
    print('run')
    exe = CExecutor(module)
    exe.engine.get_pointer_to_function(fntester)
    func = exe.get_ctype_function(fntester, 'void')

    func()


if __name__ == '__main__':
    main()

