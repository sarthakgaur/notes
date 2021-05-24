use std::path::{Path, PathBuf};

use crate::utils;
use crate::request::Request;
use crate::config::Config;

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

pub fn build_note_paths(request: &Request, config: &Config) -> NotePaths {
    let notes_dir = Path::new(&config.notes_parent_dir).join("notes");
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
