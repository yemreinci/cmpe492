import sys
import subprocess
import os
import argparse
import re
import time
import csv
import http.server
import socketserver
import threading
from datetime import datetime
from typing import List
from selenium import webdriver

class TestFailedError(BaseException):
    pass


class Platform:
    def __init__(self, build_dir: str, name='unknown'):
        self.build_dir = build_dir
        self.name = name
        os.makedirs(build_dir, exist_ok=True)
        self._do_configure()

    def _parse_running_time(self, benchmark_out: str) -> float:
        for line in benchmark_out.split('\n'):
            m = re.match(r'^running time:\s+([\d\.]+)', line)
            if m:
                return float(m.group(1))
        raise RuntimeError('unexpected benchmark output')

    def build(self, task: str, version: str):
        print(f'Building {task}_{version} for platform "{self.name}" ...')

        subprocess.check_call([
            'cmake',
            '--build', '.',
            '--target', f'test-{task}_{version}', f'bench-{task}_{version}',
            '--parallel', '2',
        ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def test(self, task: str, version: str) -> bool:
        print(f'Testing {task}_{version} on platform "{self.name}" ...')
        success = self._do_test(task, version)
        if not success:
            raise TestFailedError()

    def benchmark(self, task: str, version: str, n_repeat=1, skip_tests=False) -> List[float]:
        self.build(task, version)

        if not skip_tests:
            self.test(task, version)

        print(f'Benchmarking {task}_{version} on platform "{self.name}" ...')

        running_times = []

        for i in range(n_repeat):
            rt = self._do_benchmark(task, version)
            print(f'Benchmark {i+1}:', rt)
            running_times.append(rt)
        return running_times

    def close(self):
        self._do_close()

    def _do_configure(self):
        raise NotImplementedError

    def _do_test(self, task: str, version: str):
        raise NotImplementedError

    def _do_benchmark(self, task: str, version: str) -> float:
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

    def _do_test(self, task: str, version: str) -> bool:
        try:
            subprocess.check_call(
                [f'./{task}/test-{task}_{version}'],
                cwd=self.build_dir,
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: str) -> float:
        output = subprocess.check_output(
            [f'./{task}/bench-{task}_{version}'], cwd=self.build_dir)
        output = output.decode()
        return self._parse_running_time(output)


class BrowserBase(Platform):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.driver = self._get_driver()

    def _get_driver(self):
        raise NotImplementedError

    def _do_configure(self):
        if not os.environ.get('EMSDK'):
            raise RuntimeError('Emscripten environment is not set')

        subprocess.check_call([
            'emcmake', 'cmake',
            os.getcwd(),
            '-DCMAKE_BUILD_TYPE=Release',
            '-DCMAKE_CXX_FLAGS=-msimd128 -s TOTAL_MEMORY=1000MB -pthread -s PTHREAD_POOL_SIZE=4 -s EXIT_RUNTIME=1'
        ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

    def _run_and_get_output(self, file):
        self.driver.get(f'http://localhost:8000/{self.build_dir}/{file}')
        output_text = None
        while not output_text:
            time.sleep(0.3)
            output_el = self.driver.find_element_by_id('output')
            output_text = output_el.get_attribute('value').strip()
        return output_text

    def _do_test(self, task: str, version: str):
        output_text = self._run_and_get_output('{task}/test-{task}_{version}.html')
        return not ('FAIL' in output_text)

    def _do_benchmark(self, task: str, version: str):
        output_text = self._run_and_get_output(f'{task}/bench-{task}_{version}.html')
        return self._parse_running_time(output_text)

    def _do_close(self):
        self.driver.close()

class Chrome(BrowserBase):
    def _get_driver(self):
        options = webdriver.ChromeOptions()
        options.binary_location = '/Applications/Google Chrome Canary.app/Contents/MacOS/Google Chrome Canary'
        options.add_argument('enable-webassembly-simd')
        options.add_argument('enable-experimental-webassembly-features')
        options.add_argument('enable-webassembly-threads')
        options.add_argument('disable-webassembly-tiering')
        options.add_argument('disable-webassembly-baseline')
        options.add_argument('disable-webassembly-lazy-compilation')

        return webdriver.Chrome(options=options)

class Firefox(BrowserBase):
    def _get_driver(self):
        options = webdriver.FirefoxOptions()
        options.binary_location = '/Applications/Firefox Nightly.app/Contents/MacOS/firefox'
        profile = webdriver.FirefoxProfile()
        profile.set_preference('dom.postMessage.sharedArrayBuffer.bypassCOOP_COEP.insecure.enabled', True)
        profile.set_preference('javascript.options.wasm_baselinejit', False)
        return webdriver.Firefox(options=options, firefox_profile=profile)


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

    def _do_test(self, task: str, version: str) -> bool:
        try:
            subprocess.check_call(
                [self.wavm_exe, 'run', '--enable', 'all',
                    f'./{task}/test-{task}_{version}'],
                cwd=self.build_dir,
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: str) -> float:
        output = subprocess.check_output(
            [self.wavm_exe, 'run', '--enable', 'all',
                f'./{task}/bench-{task}_{version}'],
            cwd=self.build_dir)
        output = output.decode()
        return self._parse_running_time(output)


class Wasmer(WASIBase):
    def _do_test(self, task: str, version: str) -> bool:
        try:
            subprocess.check_call(
                ['wasmer', 'run', '--enable-all', '--llvm',
                    f'./{task}/test-{task}_{version}'],
                cwd=self.build_dir,
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: str) -> float:
        output = subprocess.check_output(
            ['wasmer', 'run', '--enable-all', '--llvm',
                f'./{task}/bench-{task}_{version}'],
            cwd=self.build_dir)
        output = output.decode()
        return self._parse_running_time(output)


class Wasmtime(WASIBase):
    def __init__(self, *args, **kwargs):
        self.wasmtime_exe = self._get_wasmtime_exe()
        super().__init__(*args, **kwargs)

    def _get_wasmtime_exe(self):
        value = os.environ.get('WASMTIME_EXE')
        if not value:
            raise RuntimeError('set environ variable WASMTIME_EXE')
        return value

    def _do_test(self, task: str, version: str) -> bool:
        try:
            subprocess.check_call(
                [f'{self.wasmtime_exe}', 'run', '--enable-all', '--cranelift',
                    f'./{task}/test-{task}_{version}'],
                cwd=self.build_dir,
                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            return True
        except subprocess.CalledProcessError:
            return False

    def _do_benchmark(self, task: str, version: str) -> float:
        output = subprocess.check_output(
            [f'{self.wasmtime_exe}', 'run', '--enable-all', '--cranelift',
                f'./{task}/bench-{task}_{version}'],
            cwd=self.build_dir)
        output = output.decode()
        return self._parse_running_time(output)


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
        parser.add_argument(f'--{task}', type=str, nargs='+')

    parser.add_argument('--platforms', type=str, nargs='+')
    parser.add_argument('--n-repeat', type=int, nargs='?', default=3)
    parser.add_argument('--skip-tests', action='store_true')

    args = parser.parse_args()

    results = []

    report_file_path = f'reports/report-{datetime.now().strftime("%Y-%m-%d-%H-%M-%S")}.csv'
    csv_file = open(report_file_path, 'w')
    results_csv = csv.writer(csv_file)
    results_csv.writerow(['platform', 'task', 'version', 'running-time'])

    for platform_name in args.platforms:
        platform = platforms[platform_name](
            f'build-{platform_name}', platform_name)

        for task in tasks:
            requested_versions = getattr(args, task)
            if not requested_versions:
                continue

            for version in requested_versions:
                try:
                    running_times = platform.benchmark(
                        task, version, n_repeat=args.n_repeat, skip_tests=args.skip_tests)
                except Exception as e:
                    print(f'Error while processing {task}_{version}: {e}')
                else:
                    for rt in running_times:
                        results_csv.writerow(
                            [platform_name, task, version, rt])
                    results.append(
                        [platform_name, task, version, running_times])

        platform.close()

    csv_file.close()

    def stat(a):
        average = sum(a) / len(a)
        var = 0
        for x in a:
            var += (x-average)**2 / len(a)
        std = var ** 0.5
        return f'{average:.3f} ± {std:.3f}'

    print('\n'.join(['\t'.join(str(e) for e in row[:-1]) +
                     '\t' + stat(row[-1]) for row in results]))


if __name__ == '__main__':
    main(sys.argv)
