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
use serde::{Deserialize, Serialize};
use tempfile::NamedTempFile;

// TODO Add note to user specified file. Done.
// TODO Open a text editor for creating notes. Done.
// TODO Open note for editing. Done.
// TODO Add option to list all notes. Done.
// TODO Add option to include date in note. Done.
// TODO Add support for templates. Done.
// TODO Refactor the code. Done.
// TODO Add config file. Done.
// TODO Add cache file. Done.
// TODO Refactor the code.

#[derive(Debug)]
enum RequestType {
    WriteNote,
    EditNote,
    ListNotes,
    SaveTemplate,
}

#[derive(Debug)]
enum NoteSource {
    CommandLine,
    StandardInput,
    Editor,
}

#[derive(Debug)]
enum FileStatus {
    Created,
    Exists,
}

#[derive(Debug)]
struct Request {
    request_type: RequestType,
    note_file_name: String,
    note_body: Option<String>,
    note_source: NoteSource,
    editor_name: Option<String>,
    write_date: bool,
    template_file_name: String,
    use_template: bool,
}

#[derive(Debug)]
struct GeneralPaths {
    home_dir: PathBuf,
    config_dir: PathBuf,
    cache_dir: PathBuf,
    default_notes_parent_dir: PathBuf,
    config_file: PathBuf,
    cache_file: PathBuf,
}

#[derive(Debug)]
struct NotePaths {
    notes_dir: PathBuf,
    templates_dir: PathBuf,
    note_file: PathBuf,
    template_file: PathBuf,
}

#[derive(Serialize, Deserialize, Debug)]
struct Config {
    notes_parent_dir: PathBuf,
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

    start(matches)
}

fn start(matches: ArgMatches) {
    let request = build_request(&matches);
    let gen_paths = build_gen_paths();

    create_dir(&gen_paths.cache_dir);
    create_dir(&gen_paths.config_dir);

    let config = build_config(&gen_paths);
    let note_paths = build_note_paths(&request, &config);

    create_dir(&note_paths.templates_dir); // Need only this.

    handle_request(request, &note_paths);
}

fn build_config(gen_paths: &GeneralPaths) -> Config {
    let config_file_status = create_file(&gen_paths.config_file);
    let cache_file_status = create_file(&gen_paths.cache_file);

    let config;
    if let FileStatus::Created = config_file_status {
        config = Config {
            notes_parent_dir: PathBuf::from(&gen_paths.default_notes_parent_dir),
        };

        write_config(gen_paths, &config);
        write_cache(gen_paths, &config)
    } else {
        if let FileStatus::Exists = cache_file_status {
            let config_file_stat = fs::metadata(&gen_paths.config_file).unwrap();
            let cache_file_stat = fs::metadata(&gen_paths.cache_file).unwrap();
            let config_mod_time = config_file_stat.modified().unwrap().elapsed().unwrap();
            let cache_mod_time = cache_file_stat.modified().unwrap().elapsed().unwrap();

            if config_mod_time < cache_mod_time {
                config = read_config(&gen_paths);
                write_cache(&gen_paths, &config)
            } else {
                config = read_cache(&gen_paths);
            }
        } else {
            config = read_config(&gen_paths);
            write_cache(&gen_paths, &config)
        }
    }

    return config;
}

fn build_request(matches: &ArgMatches) -> Request {
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

    let mut template_file_name = "template.txt".to_string();
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
    }

    let note_source;
    let mut editor_name = None;
    if let Some(_) = note_body {
        note_source = NoteSource::CommandLine;
    } else if let Ok(editor) = env::var("EDITOR") {
        note_source = NoteSource::Editor;
        editor_name = Some(editor);
    } else {
        note_source = NoteSource::StandardInput;
    }

    let note_file_name = matches.value_of("file").unwrap_or("notes.txt").to_string();
    let write_date = matches.is_present("date");
    let use_template = matches.is_present("template");

    return Request {
        request_type,
        note_file_name,
        note_body,
        note_source,
        editor_name,
        write_date,
        template_file_name,
        use_template,
    };
}

fn build_gen_paths() -> GeneralPaths {
    let home_dir = get_home_dir();
    let config_dir = Path::new(&home_dir).join(".config").join("notes");
    let cache_dir = Path::new(&home_dir).join(".cache").join("notes");
    let default_notes_parent_dir = PathBuf::from(&home_dir);
    let config_file = Path::new(&config_dir).join("config.toml");
    let cache_file = Path::new(&cache_dir).join("cache");

    return GeneralPaths {
        home_dir,
        config_dir,
        cache_dir,
        default_notes_parent_dir,
        config_file,
        cache_file,
    };
}

fn build_note_paths(request: &Request, config: &Config) -> NotePaths {
    let notes_dir = Path::new(&config.notes_parent_dir).join("notes"); // here
    let templates_dir = Path::new(&notes_dir).join("templates");
    let note_file = Path::new(&notes_dir).join(&request.note_file_name);
    let template_file = Path::new(&templates_dir).join(&request.template_file_name);

    return NotePaths {
        notes_dir,
        templates_dir,
        note_file,
        template_file,
    };
}

fn handle_request(request: Request, note_paths: &NotePaths) {
    match request.request_type {
        RequestType::WriteNote => handle_write_request(&request, &note_paths),
        RequestType::EditNote => handle_edit_request(&request, &note_paths),
        RequestType::ListNotes => handle_list_request(&note_paths),
        RequestType::SaveTemplate => handle_save_request(&request, &note_paths),
    }
}

fn handle_write_request(request: &Request, note_paths: &NotePaths) {
    if request.use_template && request.editor_name.is_none() {
        eprintln!("$EDITOR environment variable is required for using templates. Exiting...");
        process::exit(1);
    }

    let note_body = get_note_body(request, &note_paths.template_file);
    let note = create_note(request.write_date, &note_body);
    write_note(&note_paths.note_file, &note);
}

fn handle_edit_request(request: &Request, note_paths: &NotePaths) {
    if let Some(editor_name) = &request.editor_name {
        let status = open_editor(editor_name, &note_paths.note_file);
        if !status.success() {
            eprintln!("Child process failed. Exiting...");
            process::exit(1);
        }
    } else {
        eprintln!("$EDITOR environment variable is required for editing. Exiting...");
        process::exit(1);
    }
}

fn handle_list_request(note_paths: &NotePaths) {
    list_dir_contents(&note_paths.notes_dir);
}

fn handle_save_request(request: &Request, note_paths: &NotePaths) {
    if let Some(editor) = &request.editor_name {
        create_file(&note_paths.template_file);
        open_editor(editor, &note_paths.template_file);
    } else {
        eprintln!("$EDITOR env var is required for saving templates. Exiting...");
        process::exit(1);
    }
}

fn get_note_body(request: &Request, template_file_path: &PathBuf) -> String {
    let note_body;

    if let Some(v) = &request.note_body {
        note_body = v.to_string();
    } else if let NoteSource::Editor = request.note_source {
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
    let status = open_editor(
        &request.editor_name.as_ref().unwrap(),
        &temp_path.to_path_buf(),
    );

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

fn create_file(path: &PathBuf) -> FileStatus {
    if !(path.exists() && path.is_file()) {
        if let Err(_) = fs::File::create(path) {
            eprintln!("{:?} file creation failed. Exiting...", path.file_name());
            process::exit(1);
        }
        return FileStatus::Created;
    }
    return FileStatus::Exists;
}

fn list_dir_contents(path: &PathBuf) {
    let paths = fs::read_dir(path).unwrap();

    for path in paths {
        let file_name = path.unwrap().file_name();
        println!("{}", file_name.to_str().unwrap());
    }
}

fn write_config(gen_paths: &GeneralPaths, config: &Config) {
    let content = format!(
        "# Specify the absolute path of the notes parent directory.\n\
             #\n\
             # Example usage:\n\
             # notes_parent_dir = \"/home/john/Documents/\"\n\
             # \n\
             # This will create the directory \"/home/john/Documents/notes\"\n\n\

             notes_parent_dir = {:?}\n\n",
        &config.notes_parent_dir
    );

    let mut file = OpenOptions::new()
        .write(true)
        .truncate(true)
        .open(&gen_paths.config_file)
        .unwrap();

    if let Err(_) = file.write_all(content.as_bytes()) {
        eprintln!("Could not write to the config file. Exiting...");
        process::exit(1);
    }
}

fn read_config(gen_paths: &GeneralPaths) -> Config {
    let contents = fs::read_to_string(&gen_paths.config_file).unwrap();
    let value: toml::Value = contents.parse::<toml::Value>().unwrap();

    let notes_parent_dir;
    if let Some(v) = value.get("notes_parent_dir") {
        notes_parent_dir = PathBuf::from(v.as_str().unwrap());
    } else {
        notes_parent_dir = PathBuf::from(&gen_paths.default_notes_parent_dir);
    }

    let config = Config { notes_parent_dir };

    return config;
}

fn read_cache(gen_paths: &GeneralPaths) -> Config {
    let contents = fs::read_to_string(&gen_paths.cache_file).unwrap();
    let bytes = contents.as_bytes();
    let config: Config = bincode::deserialize(bytes).unwrap();
    return config;
}

fn write_cache(gen_paths: &GeneralPaths, config: &Config) {
    let bytes = bincode::serialize(config).unwrap();

    let mut cache_file = OpenOptions::new()
        .write(true)
        .truncate(true)
        .open(&gen_paths.cache_file)
        .unwrap();

    if let Err(_) = cache_file.write_all(&bytes) {
        eprintln!("Could not write to the cache file. Exiting...");
        process::exit(1);
    }
}
