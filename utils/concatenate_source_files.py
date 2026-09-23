#!/usr/bin/env python3
"""
"""

# Import standard libraries.
import pathlib
import typing
import sys

# Type aliases.
Path: typing.TypeAlias = pathlib.Path

#===================================================================================================
# Configuration
#===================================================================================================

# Files equal to or larger than this size are skipped (150 KiB by default).
MAX_FILE_SIZE: int = 150 * 1024

# The header printed before each file's contents.
HEADER_PREFIX: str = "==> "
HEADER_SUFFIX: str = " <=="


def find_files(root: Path, exclude: list[Path] | None = None) -> list[Path]:
    """
    Recursively find all regular files below the specified root directory.

    Args:
        root (Path): The root directory to search.

    Returns:
        (list[Path]): A list of regular file paths found under the root directory.

    Notes:
        Symbolic links are excluded, including symbolic links to regular files.
        The returned paths are sorted by their relative POSIX path to make the
        output deterministic.
    """
    if exclude is None:
        exclude = []

    # Find all files recursively with filtering out symlinks and non-regular files.
    files = [path for path in root.rglob("*") if not path.is_symlink() and path.is_file() and not any(exc in path.parts for exc in exclude)]
    files.sort(key=lambda path: path.relative_to(root).as_posix())
    return files


def print_message_on_top(root: Path, target_files: list[Path]) -> None:
    """
    """
    print("This file is a concatenation of the source files of a software project.")
    print("Each file is seperated by a header line like the following:")
    print("    " + HEADER_PREFIX + "relative/path/to/file" + HEADER_SUFFIX)
    print("The following is a list of source files contained in this file:")
    print_file_tree(target_files, root)
    print()


def print_file_tree(target_files: list[Path], root: Path) -> None:
    """
    Print a tree structure of the target files relative to the specified root directory.

    Args:
        target_files (list[Path]): A list of file paths to be displayed in the tree.
        root         (Path)      : The root directory for calculating the relative paths.
    """
    def print_node(node: dict[str, dict], prefix: str = "") -> None:
        """
        Print the tree structure in a human-readable format.
        """
        entries = sorted(node.items(), key=lambda item: (not bool(item[1]), item[0].casefold()))

        for index, (name, children) in enumerate(entries):
            is_last = index == len(entries) - 1

            connector = "└── " if is_last else "├── "
            print(f"{prefix}{connector}{name}")

            if children:
                child_prefix = prefix + ("    " if is_last else "│   ")
                print_node(children, child_prefix)

    # Resolve the root path to its absolute form to ensure consistent relative path calculations.
    root = root.resolve()

    # Build a tree structure to represent the directory hierarchy of the target files.
    tree: dict[str, dict] = {}

    # Populate the tree structure with the relative paths of the target files.
    for file_path in target_files:

        # Resolve the file path to its absolute form to ensure consistent relative path calculations.
        file_path = file_path.resolve()

        # Calculate the relative path of the file with respect to the root directory.
        try:
            relative_path = file_path.relative_to(root)
        except ValueError as exc:
            raise ValueError(f"Outside of root directory: {file_path}") from exc

        # Traverse the tree structure and create nested dictionaries for each part of the relative path.
        node = tree
        for part in relative_path.parts:
            node = node.setdefault(part, {})

    print(root.name or str(root))
    print_node(tree)


def print_header(relative_path: str) -> None:
    """
    Print the header for a file using its relative path.

    Args:
        relative_path (str): The relative path of the file to be printed in the header.
    """
    print(f"{HEADER_PREFIX}{relative_path}{HEADER_SUFFIX}")


def skip_file(reason: str) -> None:
    """
    Print a skip message and a blank line.
    """
    print(f"[SKIPPED: {reason}]")
    print()


def read_text_file(path: Path) -> tuple[str | None, str | None]:
    """
    Read a file as UTF-8 text and return its contents or a skip reason.
    A file containing a NUL byte is treated as a binary file. Files that
    cannot be decoded as UTF-8 are also rejected.

    Returns:
        text   (str | None): The contents of the file as a string.
        reason (str | None): A string describing the reason for skipping the file.
    """
    # Read the file as bytes first to check for binary content.
    try:
        data = path.read_bytes()
    except OSError as error:
        return (None, f"cannot read file: {error}")

    # Check for binary content by looking for NUL bytes.
    if b"\x00" in data:
        return (None, "binary file")

    # Try to decode the bytes as UTF-8 text.
    try:
        return (data.decode("utf-8"), None)
    except UnicodeDecodeError:
        return (None, "file is not valid UTF-8")


def process_file(path: Path, root: Path) -> None:
    """
    Process one file and write its header and contents to stdout.
    Files that are too large, binary, invalid UTF-8, or otherwise unreadable
    are skipped with a corresponding reason.

    Args:
        path (Path): The path of the file to process.
        root (Path): The root directory for calculating the relative path.
    """
    # Get the path relative to root in POSIX format.
    relative_path: str = path.relative_to(root).as_posix()

    # Print the header for the file.
    print_header(relative_path)

    # Check the file size and skip if it exceeds the maximum allowed size.
    try:
        file_size = path.stat().st_size
    except OSError as error:
        return skip_file(f"cannot get file size: {error}")

    # Skip the file if its size exceeds or equals the maximum allowed size.
    if file_size >= MAX_FILE_SIZE:
        return skip_file(f"file size exceeds or equals {MAX_FILE_SIZE} bytes")

    # Read the file as text and handle any errors that may occur.
    (text, error) = read_text_file(path)

    # If there was an error reading the file, skip it and print the reason.
    if error is not None:
        return skip_file(error)

    # The "text" should not be None if the control reaches here.
    assert text is not None

    # Write the file's contents to stdout.
    sys.stdout.write(text)

    # Ensure that the next header starts on a new line even when
    # the input file does not end with a newline.
    if not text.endswith("\n"):
        sys.stdout.write("\n")

    # Print a blank line for readability between files.
    print()


def main() -> None:
    """
    Recursively concatenate eligible files from the current directory.
    """
    # Get the current working directory.
    root: Path = Path.cwd()

    # Get all the target files to be processed.
    target_files: list[Path] = find_files(root, exclude=[".git"])

    # Print a top message.
    print_message_on_top(root, target_files)

    # Recursively find all eligible files and process them.
    for path in target_files:
        process_file(path, root)


if __name__ == "__main__":
    main()


# vim: expandtab tabstop=4 shiftwidth=4 fdm=marker
