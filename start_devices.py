import subprocess
import os

output_folder = 'captured_output'


def create_output_folder(folder_name=output_folder):
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)


def main():
    create_output_folder()
    osd_ports = ['4242', '4243', '4244', '4245', '4246']
    filename_pattern = f'{output_folder}/{{}}.txt'
    files = [open(filename_pattern.format(osd_port), 'w') for osd_port in osd_ports]
    for i, osd_port in enumerate(osd_ports):
        subprocess.Popen('cmake-build-debug/osd -p {}'.format(osd_port),
                         shell=True, stdout=files[i], stderr=files[i])
        print('Started {}'.format(osd_port))

    while True:
        pass


if __name__ == '__main__':
    main()
