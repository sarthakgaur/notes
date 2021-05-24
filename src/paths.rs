use std::path::PathBuf;

use crate::config::Config;
use crate::request::Request;
use crate::utils;

#[derive(Debug)]
pub struct GeneralPaths {
    pub home_dir: PathBuf,
    pub config_dir: PathBuf,
    pub cache_dir: PathBuf,
    pub default_notes_parent_dir: PathBuf,
    pub config_file: PathBuf,
    pub cache_file: PathBuf,
}

#[derive(Debug)]
pub struct NotePaths {
    pub notes_dir: PathBuf,
    pub templates_dir: PathBuf,
    pub note_file: PathBuf,
    pub template_file: PathBuf,
}

pub fn build_gen_paths() -> GeneralPaths {
    let home_dir = utils::get_home_dir();
    GeneralPaths {
        cache_dir: home_dir.join(".cache").join("notes"),
        cache_file: home_dir.join(".cache").join("notes").join("cache"),
        config_dir: home_dir.join(".config").join("notes"),
        config_file: home_dir.join(".config").join("notes").join("config.toml"),
        default_notes_parent_dir: home_dir.clone(),
        home_dir,
    }
}

pub fn build_note_paths(request: &Request, config: &Config) -> NotePaths {
    let notes_dir = config.notes_parent_dir.join("notes");
    NotePaths {
        note_file: notes_dir.join(&request.note_file_name),
        templates_dir: notes_dir.join("templates"),
        template_file: notes_dir
            .join("templates")
            .join(&request.template_file_name),
        notes_dir,
    }
}
