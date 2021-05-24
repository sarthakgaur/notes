extern crate clap;
extern crate dirs;

use clap::{App, Arg, ArgMatches};

mod cache;
mod config;
mod note;
mod paths;
mod request;
mod utils;

fn main() {
    let matches = App::new("notes")
        .version("0.2")
        .about("Make notes from command line")
        .arg(
            Arg::with_name("file")
                .short("f")
                .long("file")
                .value_name("FILE")
                .takes_value(true)
                .help("The note file to append the note to. Defaults to notes.txt."),
        )
        .arg(
            Arg::with_name("note")
                .short("n")
                .long("note")
                .takes_value(true)
                .help("The note message."),
        )
        .arg(
            Arg::with_name("edit")
                .short("e")
                .long("edit")
                .conflicts_with("note")
                .help("Open the note file for editing."),
        )
        .arg(
            Arg::with_name("list")
                .short("l")
                .long("list")
                .conflicts_with_all(&["note", "edit"])
                .help("List all the notes files in the notes directory."),
        )
        .arg(
            Arg::with_name("date")
                .short("d")
                .long("date")
                .conflicts_with_all(&["edit", "list"])
                .help("The date string will be added to the note."),
        )
        .arg(
            Arg::with_name("save_template")
                .short("s")
                .long("save-template")
                .value_name("FILE")
                .takes_value(true)
                .conflicts_with_all(&["file", "note", "edit", "list", "date"])
                .help("Create or update a template file."),
        )
        .arg(
            Arg::with_name("template")
                .short("t")
                .long("template")
                .value_name("FILE")
                .takes_value(true)
                .conflicts_with_all(&["edit", "list"])
                .help("Use the specified template."),
        )
        .get_matches();

    start(matches)
}

fn start(matches: ArgMatches) {
    let request = request::build_request(&matches);
    let gen_paths = paths::build_gen_paths();

    utils::create_dir(&gen_paths.cache_dir);
    utils::create_dir(&gen_paths.config_dir);

    let config = config::build_config(&gen_paths);
    let note_paths = paths::build_note_paths(&request, &config);

    // This will also create the notes directory.
    utils::create_dir(&note_paths.templates_dir);

    request::handle_request(request, &note_paths);
}
