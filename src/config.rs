use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::PathBuf;
use std::process;

use serde::{Deserialize, Serialize};
use toml::Value;

use crate::cache;
use crate::paths::GeneralPaths;
use crate::utils::{self, FileStatus};

#[derive(Serialize, Deserialize, Debug)]
pub struct Config {
    pub notes_parent_dir: PathBuf,
}

pub fn build_config(gen_paths: &GeneralPaths) -> Config {
    let config_file_status = utils::create_file(&gen_paths.config_file);
    let cache_file_status = utils::create_file(&gen_paths.cache_file);

    let config;
    if let FileStatus::Created = config_file_status {
        config = Config {
            notes_parent_dir: PathBuf::from(&gen_paths.default_notes_parent_dir),
        };

        write_config(gen_paths, &config);
        cache::write_cache(gen_paths, &config)
    } else if let FileStatus::Exists = cache_file_status {
        let config_file_stat = fs::metadata(&gen_paths.config_file).unwrap();
        let cache_file_stat = fs::metadata(&gen_paths.cache_file).unwrap();
        let config_mod_time = config_file_stat.modified().unwrap().elapsed().unwrap();
        let cache_mod_time = cache_file_stat.modified().unwrap().elapsed().unwrap();

        if config_mod_time < cache_mod_time {
            config = cache::update_cache(gen_paths);
        } else {
            config = cache::read_cache(&gen_paths);
        }
    } else {
        config = cache::update_cache(gen_paths);
    }

    config
}

pub fn parse_config_toml(gen_paths: &GeneralPaths, config_toml: Value) -> Config {
    let notes_parent_dir;
    if let Some(v) = config_toml.get("notes_parent_dir") {
        notes_parent_dir = PathBuf::from(v.as_str().unwrap());
    } else {
        notes_parent_dir = PathBuf::from(&gen_paths.default_notes_parent_dir);
    }

    Config { notes_parent_dir }
}

pub fn read_config(gen_paths: &GeneralPaths) -> Value {
    let contents = fs::read_to_string(&gen_paths.config_file).unwrap();
    contents.parse::<toml::Value>().unwrap()
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

    if file.write_all(content.as_bytes()).is_err() {
        eprintln!("Could not write to the config file. Exiting...");
        process::exit(1);
    }
}
