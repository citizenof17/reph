#!/usr/bin/python3

import subprocess
import sys

from util import create_output_folder, OUTPUT_FOLDER, SOURCE_ROOT_PATH


default_port = 4242
total_osd_count = 5


def get_osd_count_from_cmdline():
    if len(sys.argv) > 1:
        return int(sys.argv[1])
    return None


def main():
    create_output_folder()
    osd_count = get_osd_count_from_cmdline() or total_osd_count
    osd_ports = [str(default_port + i) for i in range(osd_count)]
    filename_pattern = f'{OUTPUT_FOLDER}/{{}}.txt'
    files = [open(filename_pattern.format(osd_port), 'w') for osd_port in osd_ports]
    base_path = f'{SOURCE_ROOT_PATH}/cmake-build-debug/{{}}'

    for i, osd_port in enumerate(osd_ports):
        subprocess.Popen(base_path.format('osd -p {}'.format(osd_port)),
                         shell=True, stdout=files[i], stderr=files[i])
        print('Started {}'.format(osd_port))

    while True:
        pass


if __name__ == '__main__':
    main()
