import os

SOURCE_ROOT_PATH = '/home/pavel/reph/'
OUTPUT_FOLDER = SOURCE_ROOT_PATH + 'captured_output'


def create_output_folder(folder_name=OUTPUT_FOLDER):
    if not os.path.exists(folder_name):
        os.makedirs(folder_name)
