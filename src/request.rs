use std::env;

use anyhow::{bail, Context};
use clap::ArgMatches;
use fehler::throws;

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

impl Request {
    pub fn new(matches: &ArgMatches) -> Request {
        let note_body = matches.value_of("note").map(str::to_owned);

        let request_type = if matches.is_present("edit") {
            RequestType::EditNote
        } else if matches.is_present("list") {
            RequestType::ListNotes
        } else if matches.is_present("save_template") {
            RequestType::SaveTemplate
        } else if matches.is_present("list_templates") {
            RequestType::ListTemplates
        } else {
            RequestType::WriteNote
        };

        let template_file_name = matches
            .value_of("save_template")
            .or_else(|| matches.value_of("template"))
            .unwrap_or("template.txt")
            .to_owned();

        let note_file_name = matches
            .value_of("file")
            .or_else(|| matches.value_of("edit"))
            .unwrap_or("notes.txt")
            .to_owned();

        let mut editor_name = None;

        let note_source = if note_body.is_some() {
            NoteSource::CommandLine
        } else if let Ok(editor) = env::var("EDITOR") {
            editor_name = Some(editor);
            NoteSource::Editor
        } else {
            NoteSource::StandardInput
        };

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

    #[throws(anyhow::Error)]
    pub fn handle(self, note_paths: &NotePaths) {
        match self.request_type {
            RequestType::WriteNote => self.handle_write_request(&note_paths)?,
            RequestType::EditNote => self.handle_edit_request(&note_paths)?,
            RequestType::ListNotes => self.handle_list_request(&note_paths)?,
            RequestType::SaveTemplate => self.handle_save_request(&note_paths)?,
            RequestType::ListTemplates => self.handle_list_templates(note_paths)?,
        }
    }

    #[throws(anyhow::Error)]
    fn handle_write_request(&self, note_paths: &NotePaths) {
        if self.use_template && self.editor_name.is_none() {
            bail!("$EDITOR environment variable is required for using templates.");
        }

        let note_body = note::get_note_body(self, &note_paths.template_file)
            .context("Failed to get note body")?;
        let note = note::create_note(self.write_date, &note_body);
        note::write_note(&note_paths.note_file, &note)?;
    }

    #[throws(anyhow::Error)]
    fn handle_edit_request(&self, note_paths: &NotePaths) {
        if let Some(editor_name) = &self.editor_name {
            let status = utils::open_editor(editor_name, &note_paths.note_file)?;
            if !status.success() {
                bail!("Child process failed.")
            }
        } else {
            bail!("Child process failed.")
        }
    }

    #[throws(anyhow::Error)]
    fn handle_list_request(&self, note_paths: &NotePaths) {
        utils::list_dir_contents(&note_paths.notes_dir)?;
    }

    #[throws(anyhow::Error)]
    fn handle_save_request(&self, note_paths: &NotePaths) {
        if let Some(editor) = &self.editor_name {
            utils::create_file(&note_paths.template_file)?;
            utils::open_editor(editor, &note_paths.template_file)?;
        } else {
            bail!("$EDITOR env var is required for saving templates")
        }
    }

    #[throws(anyhow::Error)]
    fn handle_list_templates(&self, note_paths: &NotePaths) {
        utils::list_dir_contents(&note_paths.templates_dir)?;
    }
}
