use std::convert::TryInto;
use std::env;
use std::fs;
use std::fs::OpenOptions;
use std::io::{Write};
use std::path::Path;
use std::path::PathBuf;
use std::process;

extern crate dirs;
extern crate clap;

use chrono::prelude::*;
use clap::{Arg, App, ArgMatches};
use tempfile::NamedTempFile;

// TODO Add note to user specified file. Done.
// TODO Open a text editor for creating notes. Done.

enum NoteSource {
    None,
    CommandLine,
    StandardInput,
    File,
}

struct Request {
    file_name: String,
    note_body: Option<String>,
    note_source: NoteSource,
    editor: Option<String>,
}

fn main() {
    let matches = App::new("notes")
        .version("0.1")
        .about("Make notes from command line")
        .arg(Arg::with_name("file")
            .short("f")
            .long("file")
            .value_name("FILE")
            .takes_value(true))
        .arg(Arg::with_name("note")
            .short("n")
            .long("note")
            .takes_value(true))
        .get_matches();
    
    let mut request = parse_args(&matches);
    controller(&mut request);
}

fn parse_args(matches: &ArgMatches) -> Request {
    let note_body: Option<String> = match matches.value_of("note") {
        Some(v) => Some(v.to_string()),
        _ => None
    };

    let request = Request { 
        file_name: matches.value_of("file").unwrap_or("notes.txt").to_string(),
        note_body,
        note_source: NoteSource::None,
        editor: None,
    };

    return request;
}

fn controller(request: &mut Request) {
    let home_dir = get_home_dir();
    let notes_dir = Path::new(&home_dir).join("notes");
    let note_file_path = Path::new(&notes_dir).join(&request.file_name);

    handle_note_source(request);
    let note_body = get_note_body(request);
    let note = create_note(&note_body);

    create_notes_dir(&notes_dir);
    write_note(&note_file_path, &note);
}

fn handle_note_source(request: &mut Request) {
    if let Some(_) = &request.note_body {
        request.note_source = NoteSource::CommandLine;
    } else if let Ok(editor) = env::var("EDITOR") {
        request.note_source = NoteSource::File;
        request.editor = Some(editor);
    } else {
        request.note_source = NoteSource::StandardInput;
    }
}

fn get_note_body(request: &mut Request) -> String {
    let note_body;

    if let Some(v) = &request.note_body {
        note_body = v.to_string();
    } else if let NoteSource::File = request.note_source {
        note_body = get_file_note(&request.editor.as_ref().unwrap());
    } else {
        note_body = get_stdin_note();
    }

    return note_body;
}

fn get_stdin_note() -> String {
    let mut input = String::new();
    std::io::stdin().read_line(&mut input).unwrap();
    return input;
}

fn get_file_note(editor_name: &String) -> String {
    let file = NamedTempFile::new().unwrap();
    let temp_path = file.into_temp_path();

    let status = process::Command::new(editor_name)
        .arg(&temp_path)
        .status()
        .expect("Error occurred while opening the editor command.");

    if status.success() {
        let buffer = fs::read_to_string(&temp_path);
        return buffer.unwrap();
    } else {
        eprintln!("Child process failed. Exiting...");
        process::exit(1);
    }
}

fn get_home_dir() -> PathBuf {
    match dirs::home_dir() {
        Some(path) => path,
        None => {
            eprintln!("Could not get your home directory. Exiting...");
            process::exit(1);
        }
    }
}

fn create_notes_dir(home_dir: &PathBuf) {
    if let Err(_e) = fs::create_dir_all(home_dir) {
        eprintln!("Could note create notes directory.");
        process::exit(1);
    }
}

fn get_date_time_string() -> String {
    const WEEKDAYS: [&str; 7] = [
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thrusday",
        "Friday",
        "Saturday",
        "Sunday",
    ];

    let dt = chrono::prelude::Local::now();
    let day_num: usize = dt.weekday().num_days_from_monday().try_into().unwrap();

    return format!("{}, {}", WEEKDAYS[day_num], dt.format("%Y-%m-%d %H:%M"));
}

fn create_note(note_body: &String) -> String {
    return format!("{}\n{}\n\n", get_date_time_string(), note_body);
}

fn write_note(path: &PathBuf, note: &String) {
    create_note_file(path);

    let mut file = OpenOptions::new().append(true).open(path).unwrap();

    if let Err(_e) = file.write_all(note.as_bytes()) {
        eprintln!("Could not write to the file. Exiting...");
        process::exit(1);
    }
}

fn create_note_file(path: &PathBuf) {
    if !(path.exists() && path.is_file()) {
        if let Err(_e) = fs::File::create(path) {
            eprintln!("Note file creation failed. Exiting...");
            process::exit(1);
        }
    }
}
