import sys
import subprocess
import os
import argparse
import re
import logging
import http.server
import socketserver
import threading
import time
from selenium import webdriver


class Platform:
    def __init__(self, build_dir: str, name='unknown'):
        self.build_dir = build_dir
        self.name = name
        os.makedirs(build_dir, exist_ok=True)
        self._do_configure()

    def _parse_runtime(self, benchmark_out: str) -> float:
        m = re.match(r'^runtime:\s+([\d\.]+)', benchmark_out)
        return float(m.group(1))

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
        return self._do_test(task, version)

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
        except CalledProcessError:
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

        for line in output.split('\n'):
            if line.startswith('runtime:'):
                return self._parse_runtime(line)
        
        raise RuntimeError('unexpected output')


class Chrome(BrowserBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs, browser='chrome')


class Firefox(BrowserBase):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs, browser='firefox')
    

def main(argv):
    tasks = ['mm', 'conv']
    platforms = {
        'native': Native,
        'chrome': Chrome,
        'firefox': Firefox,
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
                runtime = platform.benchmark(task, version, best_of=args.best_of, skip_tests=args.skip_tests)

                results.append([platform_name, task, version, runtime])

        platform.close()

    print('\n'.join(['\t'.join(str(e) for e in row) for row in results]))

if __name__ == '__main__':
    main(sys.argv)