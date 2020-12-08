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

    def benchmark(self, task: str, version: int, best_of=1) -> float:
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

class Chrome(Platform):
    def _do_configure(self):
        subprocess.check_call([
            'emcmake', 'cmake',
            os.getcwd(),
            '-DCMAKE_BUILD_TYPE=Release', 
            '-DCMAKE_CXX_FLAGS=-msimd128  -D__SSE__  -s TOTAL_MEMORY=100MB -pthread -s PTHREAD_POOL_SIZE=4',
            '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -DNDEBUG'
        ], cwd=self.build_dir, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

        build_dir = self.build_dir

        class Handler(http.server.SimpleHTTPRequestHandler):
            def __init__(self, *args, **kwargs):
                kwargs['directory'] = build_dir
                super().__init__(*args, **kwargs)

        self.server = socketserver.TCPServer(('', 8000), Handler)

        def serve():
            self.server.serve_forever()

        self.server_thread = threading.Thread(target=serve)
        self.server_thread.start()

        options = webdriver.ChromeOptions()
        options.add_argument('enable-webassembly-simd')
        options.add_argument('enable-experimental-webassembly-features')

        self.driver = webdriver.Chrome(options=options)

    def _do_test(self, task: str, version: int):
        self.driver.get(f'http://localhost:8000/{task}/test-{task}{version}.html')
        
        while True:
            output_el = self.driver.find_element_by_id('output')
            text = output_el.get_attribute('value').strip()
            if not text:
                time.sleep(1)
            else:
                break

        return not ('FAIL' in text)

    def _do_benchmark(self, task: str, version: int):
        self.driver.get(f'http://localhost:8000/{task}/bench-{task}{version}.html')
        
        while True:
            output_el = self.driver.find_element_by_id('output')
            text = output_el.get_attribute('value').strip()
            if not text:
                time.sleep(1)
            else:
                break

        return self._parse_runtime(text)

    def _do_close(self):
        self.driver.close()
        self.server.shutdown()
        self.server.server_close()
        self.server_thread.join()


def main(argv):
    tasks = ['mm']
    platforms = {
        'native': Native,
        'chrome': Chrome
    }

    parser = argparse.ArgumentParser()
    for task in tasks:
        parser.add_argument(f'--{task}', type=int, nargs='+')

    parser.add_argument(f'--platforms', type=str, nargs='+')    
    parser.add_argument(f'--best-of', type=int, nargs='?', default=3)

    args = parser.parse_args()

    results = []

    for platform_name in args.platforms:
        platform = platforms[platform_name](f'build-{platform_name}', platform_name)

        for task in tasks:
            if not hasattr(args, task):
                continue
            requested_versions = getattr(args, task)

            for version in requested_versions:
                runtime = platform.benchmark(task, version, best_of=args.best_of)

                results.append([platform_name, task, version, runtime])

        platform.close()

    print('\n'.join(['\t'.join(str(e) for e in row) for row in results]))

if __name__ == '__main__':
    main(sys.argv)