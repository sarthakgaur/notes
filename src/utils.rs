use std::fs;
use std::path::PathBuf;
use std::process;
use std::{convert::TryInto, path::Path};

use chrono::Datelike;

#[derive(Debug)]
pub enum FileStatus {
    Created,
    Exists,
}

pub fn open_editor(editor_name: &str, file_path: &Path) -> process::ExitStatus {
    process::Command::new(editor_name)
        .arg(file_path)
        .status()
        .expect("Error occurred while opening the editor command.")
}

pub fn get_home_dir() -> PathBuf {
    match dirs::home_dir() {
        Some(path) => path,
        None => {
            eprintln!("Could not get your home directory. Exiting...");
            process::exit(1);
        }
    }
}

pub fn create_dir(path: &Path) {
    if fs::create_dir_all(path).is_err() {
        eprintln!("Could not create {:?} directory.", path.file_name());
        process::exit(1);
    }
}

pub fn get_date_time_string() -> String {
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

    format!("{}, {}", WEEKDAYS[day_num], dt.format("%Y-%m-%d %H:%M"))
}

pub fn create_file(path: &Path) -> FileStatus {
    if path.exists() && path.is_file() {
        FileStatus::Exists
    } else {
        if fs::File::create(path).is_err() {
            eprintln!("{:?} file creation failed. Exiting...", path.file_name());
            process::exit(1);
        }

        FileStatus::Created
    }
}

pub fn list_dir_contents(path: &Path) {
    for path in fs::read_dir(path).unwrap() {
        let file_name = path.unwrap().file_name();
        println!("{}", file_name.to_str().unwrap());
    }
}
