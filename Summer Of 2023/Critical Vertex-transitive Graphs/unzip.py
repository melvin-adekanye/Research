
import os
import tarfile


"""
Only Run On Ubuntu Machines
"""

# Get the source and destination paths
SOURCE_PATH = f'{os.getcwd()}/DATA'

directory = os.fsencode(SOURCE_PATH)
for folder in os.listdir(directory):

    sub_folder = os.fsdecode(folder)

    # Open the sub folder in DATA
    sub_path = f'{SOURCE_PATH}/{sub_folder}'

    sub_directory = os.fsencode(sub_path)
    for file in os.listdir(sub_directory):

        filename = os.fsdecode(file)

        new_str = str(sub_directory) + "/" + str(filename)
        new_str = new_str.replace("'", "")[1:]

        os.system(f'gzip -d "{new_str}"')
