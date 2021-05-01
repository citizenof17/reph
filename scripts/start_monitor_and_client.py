#!/usr/bin/python3

import subprocess
from util import create_output_folder, OUTPUT_FOLDER, SOURCE_ROOT_PATH


def main():
    """
        Build and run monitor + single client
    """

    build_cmd = ['/snap/clion/151/bin/cmake/linux/bin/cmake', '--build', '/home/pavel/reph/cmake-build-debug',
                 '--target']
    subprocess.call(build_cmd + ['monitor'])
    print("Built monitor")
    subprocess.call(build_cmd + ['client'])
    print("Built client")

    create_output_folder()
    filename_pattern = f'{OUTPUT_FOLDER}/{{}}.txt'
    base_path = f'{SOURCE_ROOT_PATH}/cmake-build-debug/{{}}'

    monitor_file = open(filename_pattern.format('monitor'), 'w')
    subprocess.Popen(base_path.format('monitor'), shell=True, stdout=monitor_file, stderr=monitor_file)
    print('Started monitor')

    client_file = open(filename_pattern.format('client'), 'w')
    subprocess.Popen(base_path.format('client'), shell=True, stdout=client_file, stderr=client_file)
    print('Started client')

    while True:
        pass


if __name__ == '__main__':
    main()
