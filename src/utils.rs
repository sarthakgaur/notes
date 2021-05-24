use anyhow::anyhow;
use chrono::Datelike;
use std::fs;
use std::path::PathBuf;
use std::process;
use std::{convert::TryInto, path::Path};

#[derive(Debug)]
pub enum FileStatus {
    Created,
    Exists,
}

pub fn open_editor(editor_name: &str, file_path: &Path) -> anyhow::Result<process::ExitStatus> {
    Ok(process::Command::new(editor_name).arg(file_path).status()?)
}

pub fn get_home_dir() -> anyhow::Result<PathBuf> {
    dirs::home_dir().ok_or_else(|| anyhow!("Could not get your home directory."))
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

pub fn create_file(path: &Path) -> anyhow::Result<FileStatus> {
    Ok(if path.exists() && path.is_file() {
        FileStatus::Exists
    } else {
        fs::File::create(path)?;

        FileStatus::Created
    })
}

pub fn list_dir_contents(path: &Path) -> anyhow::Result<()> {
    for path in fs::read_dir(path)? {
        let file_name = path?.file_name();

        println!(
            "{}",
            file_name
                .to_str()
                .ok_or_else(|| anyhow!("Non UTF-8 file name"))?
        );
    }

    Ok(())
}
