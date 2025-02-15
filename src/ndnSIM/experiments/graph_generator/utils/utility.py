import os


def check_and_create_dir(path):
    """
    Check if the specified directory exists, and create it if it does not.

    :param path: Path to the directory to check and possibly create.
    :return: None
    """
    if not os.path.exists(path):
        os.makedirs(path)
        print(f"Output directory is created at: {path}")
    else:
        print(f"Output directory already exists at: {path}")