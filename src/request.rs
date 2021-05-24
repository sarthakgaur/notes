use std::env;
use std::process;

use clap::ArgMatches;

use crate::note;
use crate::paths::NotePaths;
use crate::utils;

#[derive(Debug)]
pub enum RequestType {
    WriteNote,
    EditNote,
    ListNotes,
    SaveTemplate,
    ListTemplates,
}

#[derive(Debug)]
pub enum NoteSource {
    CommandLine,
    StandardInput,
    Editor,
}

#[derive(Debug)]
pub struct Request {
    pub request_type: RequestType,
    pub note_file_name: String,
    pub note_body: Option<String>,
    pub note_source: NoteSource,
    pub editor_name: Option<String>,
    pub write_date: bool,
    pub template_file_name: String,
    pub use_template: bool,
}

pub fn build_request(matches: &ArgMatches) -> Request {
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
    } else if matches.is_present("list_templates") {
        request_type = RequestType::ListTemplates;
    } else {
        request_type = RequestType::WriteNote;
    }

    let mut template_file_name = "template.txt".to_string();
    if matches.is_present("save_template") {
        template_file_name = matches
            .value_of("save_template")
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
    if note_body.is_some() {
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

    Request {
        request_type,
        note_file_name,
        note_body,
        note_source,
        editor_name,
        write_date,
        template_file_name,
        use_template,
    }
}

pub fn handle_request(request: Request, note_paths: &NotePaths) {
    match request.request_type {
        RequestType::WriteNote => handle_write_request(&request, &note_paths),
        RequestType::EditNote => handle_edit_request(&request, &note_paths),
        RequestType::ListNotes => handle_list_request(&note_paths),
        RequestType::SaveTemplate => handle_save_request(&request, &note_paths),
        RequestType::ListTemplates => handle_list_templates(note_paths),
    }
}

fn handle_write_request(request: &Request, note_paths: &NotePaths) {
    if request.use_template && request.editor_name.is_none() {
        eprintln!("$EDITOR environment variable is required for using templates. Exiting...");
        process::exit(1);
    }

    let note_body = note::get_note_body(request, &note_paths.template_file);
    let note = note::create_note(request.write_date, &note_body);
    note::write_note(&note_paths.note_file, &note);
}

fn handle_edit_request(request: &Request, note_paths: &NotePaths) {
    if let Some(editor_name) = &request.editor_name {
        let status = utils::open_editor(editor_name, &note_paths.note_file);
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
    utils::list_dir_contents(&note_paths.notes_dir);
}

fn handle_save_request(request: &Request, note_paths: &NotePaths) {
    if let Some(editor) = &request.editor_name {
        utils::create_file(&note_paths.template_file);
        utils::open_editor(editor, &note_paths.template_file);
    } else {
        eprintln!("$EDITOR env var is required for saving templates. Exiting...");
        process::exit(1);
    }
}

fn handle_list_templates(note_paths: &NotePaths) {
    utils::list_dir_contents(&note_paths.templates_dir);
}
