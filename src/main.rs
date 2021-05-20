use std::convert::TryInto;
use std::env;
use std::fs;
use std::fs::OpenOptions;
use std::io::Write;
use std::path::Path;
use std::path::PathBuf;
use std::process;

extern crate clap;
extern crate dirs;

use chrono::prelude::*;
use clap::{App, Arg, ArgMatches};
use tempfile::NamedTempFile;

// TODO Add note to user specified file. Done.
// TODO Open a text editor for creating notes. Done.
// TODO Open note for editing. Done.
// TODO Add option to list all notes. Done.
// TODO Add option to include date in note. Done.
// TODO Add support for templates.
// TODO Add config file.

#[derive(Debug)]
enum RequestType {
    WriteNote,
    EditNote,
    ListNotes,
    SaveTemplate,
}

#[derive(Debug)]
enum NoteSource {
    None,
    CommandLine,
    StandardInput,
    File,
}

#[derive(Debug)]
struct Request {
    request_type: RequestType,
    note_file_name: String,
    note_body: Option<String>,
    note_source: NoteSource,
    editor: Option<String>,
    write_date: bool,
    template_file_name: String,
    use_template: bool,
}

fn main() {
    let matches = App::new("notes")
        .version("0.1")
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
    let mut request = parse_args(&matches);
    controller(&mut request);
}

fn parse_args(matches: &ArgMatches) -> Request {
    let note_body: Option<String> = match matches.value_of("note") {
        Some(v) => Some(v.to_string()),
        _ => None,
    };

    let request_type;
    if matches.is_present("edit") {
        request_type = RequestType::EditNote;
    } else if matches.is_present("list") {
        request_type = RequestType::ListNotes;
    } else if matches.is_present("save_template") {
        request_type = RequestType::SaveTemplate;
    } else {
        request_type = RequestType::WriteNote;
    }

    let template_file_name;
    if matches.is_present("save_template") {
        template_file_name = matches
            .value_of("template")
            .unwrap_or("template.txt")
            .to_string();
    } else if matches.is_present("template") {
        template_file_name = matches
            .value_of("template")
            .unwrap_or("template.txt")
            .to_string();
    } else {
        template_file_name = "template.txt".to_string();
    }

    let note_file_name = matches.value_of("file").unwrap_or("notes.txt").to_string();
    let write_date = matches.is_present("date");
    let use_template = matches.is_present("template");

    let request = Request {
        request_type,
        note_file_name,
        note_body,
        note_source: NoteSource::None,
        editor: None,
        write_date,
        template_file_name,
        use_template,
    };

    return request;
}

fn controller(request: &mut Request) {
    let home_dir = get_home_dir();
    let notes_dir = Path::new(&home_dir).join("notes");
    let templates_dir = Path::new(&notes_dir).join("templates");
    let note_file_path = Path::new(&notes_dir).join(&request.note_file_name);
    let template_file_path = Path::new(&templates_dir).join(&request.template_file_name);

    create_dir(&notes_dir);
    create_dir(&templates_dir);
    handle_note_source(request);

    match request.request_type {
        RequestType::WriteNote => {
            handle_write_request(&request, &note_file_path, &template_file_path);
        }
        RequestType::EditNote => {
            if let Some(editor) = &request.editor {
                let status = open_editor(editor, &note_file_path);
                if !status.success() {
                    eprintln!("Child process failed. Exiting...");
                    process::exit(1);
                }
            } else {
                eprintln!("$EDITOR environment variable is required for editing. Exiting...");
                process::exit(1);
            }
        }
        RequestType::ListNotes => {
            list_notes(&notes_dir);
        }
        RequestType::SaveTemplate => {
            if let Some(editor) = &request.editor {
                save_template(editor, &template_file_path);
            } else {
                eprintln!("$EDITOR env var is required for saving templates. Exiting...");
                process::exit(1);
            }
        }
    }
}

fn handle_write_request(request: &Request, note_file_path: &PathBuf, template_file_path: &PathBuf) {
    if request.use_template && request.editor.is_none() {
        eprintln!("$EDITOR environment variable is required for using templates. Exiting...");
        process::exit(1);
    }

    let note_body = get_note_body(request, template_file_path);
    let note = create_note(request.write_date, &note_body);
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

fn get_note_body(request: &Request, template_file_path: &PathBuf) -> String {
    let note_body;

    if let Some(v) = &request.note_body {
        note_body = v.to_string();
    } else if let NoteSource::File = request.note_source {
        note_body = get_file_note(request, template_file_path);
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

fn get_file_note(request: &Request, template_file_path: &PathBuf) -> String {
    let mut file = NamedTempFile::new().unwrap();

    if request.use_template {
        let template_contents = fs::read_to_string(template_file_path).unwrap();
        file.write_all(template_contents.as_bytes()).unwrap();
    }

    let temp_path = file.into_temp_path();
    let status = open_editor(&request.editor.as_ref().unwrap(), &temp_path.to_path_buf());

    if status.success() {
        let mut buffer = fs::read_to_string(&temp_path).unwrap();
        buffer.pop(); // To remove last new line.
        return buffer;
    } else {
        eprintln!("Child process failed. Exiting...");
        process::exit(1);
    }
}

fn open_editor(editor_name: &String, file_path: &PathBuf) -> process::ExitStatus {
    let status = process::Command::new(editor_name)
        .arg(file_path)
        .status()
        .expect("Error occurred while opening the editor command.");

    return status;
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

fn create_dir(path: &PathBuf) {
    if let Err(_) = fs::create_dir_all(path) {
        eprintln!("Could note create {:?} directory.", path.file_name());
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

fn create_note(write_date: bool, note_body: &String) -> String {
    if write_date {
        return format!("{}\n{}\n\n", get_date_time_string(), note_body);
    } else {
        return format!("{}\n\n", note_body);
    }
}

fn write_note(path: &PathBuf, note: &String) {
    create_file(path);

    let mut file = OpenOptions::new().append(true).open(path).unwrap();

    if let Err(_) = file.write_all(note.as_bytes()) {
        eprintln!("Could not write to the file. Exiting...");
        process::exit(1);
    }
}

fn create_file(path: &PathBuf) {
    if !(path.exists() && path.is_file()) {
        if let Err(_) = fs::File::create(path) {
            eprintln!("{:?} file creation failed. Exiting...", path.file_name());
            process::exit(1);
        }
    }
}

fn list_notes(path: &PathBuf) {
    let paths = fs::read_dir(path).unwrap();

    for path in paths {
        let file_name = path.unwrap().file_name();
        println!("{}", file_name.to_str().unwrap());
    }
}

fn save_template(editor_name: &String, path: &PathBuf) {
    create_file(path);
    open_editor(editor_name, path);
}
