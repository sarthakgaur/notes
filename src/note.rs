use std::io::{self, Write};

use std::{
    fs::{self, OpenOptions},
    path::Path,
};

use anyhow::bail;
use fehler::throws;
use tempfile::NamedTempFile;

use crate::request::{NoteSource, Request};
use crate::utils;

pub fn create_note(write_date: bool, note_body: &str) -> String {
    if write_date {
        format!("{}\n{}\n\n", utils::get_date_time_string(), note_body)
    } else {
        format!("{}\n\n", note_body)
    }
}

#[throws(anyhow::Error)]
pub fn write_note(path: &Path, note: &str) {
    utils::create_file(path)?;

    let mut file = OpenOptions::new().append(true).open(path).unwrap();

    file.write_all(note.as_bytes())?;
}

#[throws(anyhow::Error)]
pub fn get_note_body(request: &Request, template_file_path: &Path) -> String {
    let note_body = if let Some(v) = &request.note_body {
        v.to_owned()
    } else if let NoteSource::Editor = request.note_source {
        get_file_note(request, template_file_path)?
    } else {
        get_stdin_note()
    };

    note_body.trim().to_owned()
}

#[throws(anyhow::Error)]
fn get_file_note(request: &Request, template_file_path: &Path) -> String {
    let mut file = NamedTempFile::new()?;

    if request.use_template {
        let template_contents = fs::read_to_string(template_file_path)?;

        file.write_all(template_contents.as_bytes())?;
    }

    let temp_path = file.into_temp_path();
    let status = utils::open_editor(
        &request.editor_name.as_ref().unwrap(),
        &temp_path.to_path_buf(),
    )?;

    if status.success() {
        fs::read_to_string(&temp_path)?
    } else {
        bail!("Child process failed. Exiting...")
    }
}

fn get_stdin_note() -> String {
    let mut stdout = io::stdout();
    write!(&mut stdout, "Enter note: ").unwrap();
    stdout.flush().unwrap();

    let mut input = String::new();
    std::io::stdin().read_line(&mut input).unwrap();
    input
}
