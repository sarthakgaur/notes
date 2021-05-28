# notes
Write notes from the command line.

## Installation
1. `git clone https://github.com/sarthakgaur/notes`
2. `cd notes`
2. `cargo build --release`
3. The executable is located in `target/release`

## Features
1. Quickly save notes from the command line.
2. The notes can also be written using the editor of your choice.
3. Open the note files for editing or reading.
4. Add current date and time to your notes.
5. Option to use templates in your notes.
6. Users can choose where their notes are saved.
7. List all the note and template files.

## Command Line Arguments
    USAGE:
        notes [FLAGS] [OPTIONS]

    FLAGS:
        -d, --date              The date string will be added to the note.
        -h, --help              Prints help information
        -l, --list              List all the notes files in the notes directory.
            --list-templates    List all the templates files in the templates directory.
        -V, --version           Prints version information

    OPTIONS:
        -e, --edit <FILE>             Open the note file for editing. Defaults to notes.txt
        -f, --file <FILE>             The note file to append the note to. Defaults to notes.txt.
        -n, --note <note>             The note message.
        -s, --save-template <FILE>    Create or update a template file.
        -t, --template <FILE>         Use the specified template.

## Important
1. In order to use your editor for notes, set the `EDITOR` environment variable.
2. The `-e`, `-s`, `-t` options will only work if `EDITOR` is set.
3. If the `-n` option is not provided and no conflicting options are used, the program will do the following:
    1. If the `EDITOR` environment variable is set, it will open your editor.
    2. If the environment variable is not set, it will take the note from `stdin`.
4. The program will create a `notes` directory in your `HOME` directory by default.
5. You can change the `notes` directory location by changing it in the config file.
6. The config file's path is `~/.config/notes/config.toml`.

## Configuration
The config file is located at `~/.config/notes/config.toml`. It includes the following content:

    # Specify the absolute path of the notes parent directory.
    #
    # Example usage:
    # notes_parent_dir = "/home/john/Documents/"
    # 
    # This will create the directory "/home/john/Documents/notes"

    notes_parent_dir = "/home/john"

Here you can change the `notes` directory location.

## Example Usage
Command: `notes -f test.txt -n "First note" -d`

This command will add `"First note"` to the `text.txt` file. The note will also include the current date and time.
