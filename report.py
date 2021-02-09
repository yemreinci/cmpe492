import sys
import subprocess
import os
import argparse
import re
import time

class TestFailedError(BaseException):
    pass

class Platform:
    def __init__(self, build_dir: str, name='unknown'):
        self.build_dir = build_dir
        self.name = name
        os.makedirs(build_dir, exist_ok=True)
        self._do_configure()

    def _parse_runtime(self, benchmark_out: str) -> float:
        for line in benchmark_out.split('\n'):
            m = re.match(r'^runtime:\s+([\d\.]+)', line)
            if m:
                return float(m.group(1))
        raise RuntimeError('unexpected benchmark output')

    def build(self, task: str, version: int):
        print(f'Building {task}{version} for platform "{self.name}" ...')
        
        subprocess.check_call([
            'cmake',
            '--build', '.',
            '--target', f'test-{task}{version}', f'bench-{task}{version}',
            '--parallel', '2',
        ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def test(self, task: str, version: int) -> bool:
        self.build(task, version)
        
        print(f'Testing {task}{version} on platform "{self.name}" ...')
        success = self._do_test(task, version)
        if not success:
            raise TestFailedError()

    def benchmark(self, task: str, version: int, best_of=1, skip_tests=False) -> float:
        if not skip_tests:
            self.test(task, version)
            
        print(f'Benchmarking {task}{version} on platform "{self.name}" ...')

        best_runtime = None
        for i in range(best_of):
            runtime = self._do_benchmark(task, version)
            print(f'Benchmark {i+1}:', runtime)
            if best_runtime is None or runtime < best_runtime:
                best_runtime = runtime
        return best_runtime

    def close(self):
        self._do_close()

    def _do_configure(self):
        raise NotImplementedError

    def _do_test(self, task: str, version: int):
        raise NotImplementedError

    def _do_benchmark(self, task: str, version: int) -> float:
        raise NotImplementedError

    def _do_close(self):
        pass


class Native(Platform):
    def _do_configure(self):
        subprocess.check_call([
            'cmake',
            os.getcwd(),
            '-DCMAKE_BUILD_TYPE=Release', 
            '-DCMAKE_CXX_FLAGS=-march=native',
        ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def _do_test(self, task: str, version: int) -> bool:
        try:
            subprocess.check_call(
                [f'./{task}/test-{task}{version}'],
                cwd=self.build_dir, 
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: int) -> float:
        output = subprocess.check_output(
            [f'./{task}/bench-{task}{version}'], cwd=self.build_dir)
        output = output.decode()
        runtime = self._parse_runtime(output)
        return runtime

class BrowserBase(Platform):
    def __init__(self, *args, **kwargs):
        self.browser = kwargs.pop('browser')
        super().__init__(*args, **kwargs)

    def _do_configure(self):
        subprocess.check_call([
            'emcmake', 'cmake',
            os.getcwd(),
            '-DCMAKE_BUILD_TYPE=Release', 
            '-DCMAKE_CXX_FLAGS=-msimd128  -D__SSE__  -s TOTAL_MEMORY=1000MB -pthread -s PTHREAD_POOL_SIZE=4 -s EXIT_RUNTIME=1 --emrun',
            '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG'
        ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def _do_test(self, task: str, version: int):
        try:
            subprocess.check_call([
                'emrun',
                '--browser', self.browser,
                f'{task}/test-{task}{version}.html',
                '--kill_exit'
            ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError:
            return False
        return True

    def _do_benchmark(self, task: str, version: int):
        output = subprocess.check_output([
            'emrun',
            '--browser', 'chrome',
            f'{task}/bench-{task}{version}.html',
            '--kill_exit'
        ], cwd=self.build_dir)

        output = output.decode()
        return self._parse_runtime(output)

class Chrome(BrowserBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs, browser='chrome')


class Firefox(BrowserBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs, browser='firefox')
    
class WASIBase(Platform):
    def __init__(self, *args, **kwargs):
        self.wasi_sdk_prefix = self._get_wasi_sdk_prefix()
        
        super().__init__(*args, **kwargs)

    def _get_wasi_sdk_prefix(self):
        value = os.environ.get('WASI_SDK_PREFIX')
        if not value:
            raise RuntimeError('set environment variable WASI_SDK_PREFIX')
        return os.path.abspath(value)

    def _do_configure(self):
        subprocess.check_call([
            'cmake',
            os.getcwd(),
            f'-DWASI_SDK_PREFIX={self.wasi_sdk_prefix}',
            f'-DCMAKE_BUILD_TYPE=Release', 
            f'-DCMAKE_TOOLCHAIN_FILE={self.wasi_sdk_prefix}/share/cmake/wasi-sdk.cmake',
            '-DCMAKE_C_COMPILER_WORKS=1',
            '-DCMAKE_CXX_COMPILER_WORKS=1',
            f'-DCMAKE_CXX_FLAGS=-fno-exceptions --sysroot {self.wasi_sdk_prefix}/share/wasi-sysroot -msimd128'
        ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)


class WAVM(WASIBase):
    def __init__(self, *args, **kwargs):
        self.wavm_prefix = self._get_wavm_prefix()
        self.wavm_exe = f'{self.wavm_prefix}/bin/wavm'

        super().__init__(*args, **kwargs)

    def _get_wavm_prefix(self):
        value = os.environ.get('WAVM_PREFIX')
        if not value:
            raise RuntimeError('set environment variable WAVM_PREFIX')
        return os.path.abspath(value)

    def _do_test(self, task: str, version: int) -> bool:
        try:
            subprocess.check_call(
                [self.wavm_exe, 'run', '--enable', 'all', f'./{task}/test-{task}{version}'],
                cwd=self.build_dir, 
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: int) -> float:
        output = subprocess.check_output(
            [self.wavm_exe, 'run', '--enable', 'all', f'./{task}/bench-{task}{version}'],
            cwd=self.build_dir)
        output = output.decode()
        runtime = self._parse_runtime(output)
        return runtime

class Wasmer(WASIBase):
    def _do_test(self, task: str, version: int) -> bool:
        try:
            subprocess.check_call(
                ['wasmer', 'run', '--enable-all', f'./{task}/test-{task}{version}'],
                cwd=self.build_dir, 
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: int) -> float:
        output = subprocess.check_output(
            ['wasmer', 'run', '--enable-all', f'./{task}/bench-{task}{version}'],
            cwd=self.build_dir)
        output = output.decode()
        runtime = self._parse_runtime(output)
        return runtime

class Wasmtime(WASIBase):
    def __init__(self, *args, **kwargs):
        self.wasmtime_exe = self._get_wasmtime_exe()
        super().__init__(*args, **kwargs)

    def _get_wasmtime_exe(self):
        value = os.environ.get('WASMTIME_EXE')
        if not value:
            raise RuntimeError('set environ variable WASMTIME_EXE')
        return value

    def _do_test(self, task: str, version: int) -> bool:
        try:
            subprocess.check_call(
                [f'{self.wasmtime_exe}', 'run', '--enable-all', f'./{task}/test-{task}{version}'],
                cwd=self.build_dir, 
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: int) -> float:
        output = subprocess.check_output(
            [f'{self.wasmtime_exe}', 'run', '--enable-all', f'./{task}/bench-{task}{version}'],
            cwd=self.build_dir)
        output = output.decode()
        runtime = self._parse_runtime(output)
        return runtime


def main(argv):
    tasks = ['mm', 'conv']
    platforms = {
        'native': Native,
        'chrome': Chrome,
        'firefox': Firefox,
        'wavm': WAVM,
        'wasmer': Wasmer,
        'wasmtime': Wasmtime
    }

    parser = argparse.ArgumentParser()
    for task in tasks:
        parser.add_argument(f'--{task}', type=int, nargs='+')

    parser.add_argument('--platforms', type=str, nargs='+')    
    parser.add_argument('--best-of', type=int, nargs='?', default=3)
    parser.add_argument('--skip-tests', action='store_true')

    args = parser.parse_args()

    results = []

    for platform_name in args.platforms:
        platform = platforms[platform_name](f'build-{platform_name}', platform_name)

        for task in tasks:
            requested_versions = getattr(args, task)
            if not requested_versions:
                continue

            for version in requested_versions:
                try:
                    runtime = platform.benchmark(task, version, best_of=args.best_of, skip_tests=args.skip_tests)
                except TestFailedError:
                    print('Failing tests, quiting!')
                    exit(1)

                results.append([platform_name, task, version, runtime])

        platform.close()

    print('\n'.join(['\t'.join(str(e) for e in row) for row in results]))

if __name__ == '__main__':
    main(sys.argv)