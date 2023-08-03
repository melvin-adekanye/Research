
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
    new_sub_path = sub_path

    # Try to rename the file
    try:
        new_sub_path = str(sub_path).replace("alltrans", "graph-")
    except:
        pass

    os.rename(sub_path, new_sub_path)
    sub_directory = os.fsencode(new_sub_path)

    for file in os.listdir(sub_directory):

        filename = os.fsdecode(file)
        new_filename = filename

        # Try to rename the file
        try:
            new_filename = str(filename).replace("alltrans", "graph-")
        except:
            pass

        new_str = str(sub_directory) + "/" + str(filename)
        new_filename_str = str(sub_directory) + "/" + str(new_filename)
        new_str = new_str.replace("'", "")[1:]
        new_filename_str = new_filename_str.replace("'", "")[1:]

        os.rename(new_str, new_filename_str)
        os.system(f'gzip -d "{new_filename_str}"')
