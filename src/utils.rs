use std::fs;
use std::path::PathBuf;
use std::process;
use std::{convert::TryInto, path::Path};

use anyhow::{anyhow, bail, Context};
use chrono::Datelike;
use fehler::throws;

#[derive(Debug)]
pub enum FileStatus {
    Created,
    Exists,
}

#[throws(anyhow::Error)]
pub fn open_editor(editor_name: &str, file_path: &Path) {
    let status = process::Command::new(editor_name)
        .arg(file_path)
        .status()
        .context("Failed to open editor.")?;

    if !status.success() {
        bail!("Editor process failed.")
    }
}

#[throws(anyhow::Error)]
pub fn get_home_dir() -> PathBuf {
    dirs::home_dir().ok_or_else(|| anyhow!("Could not get your home directory."))?
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

#[throws(anyhow::Error)]
pub fn create_file(path: &Path) -> FileStatus {
    if path.exists() && path.is_file() {
        FileStatus::Exists
    } else {
        fs::File::create(path).with_context(|| format!("Failed to create file at {:?}", path))?;

        FileStatus::Created
    }
}

#[throws(anyhow::Error)]
pub fn list_dir_contents(path: &Path) {
    let paths =
        fs::read_dir(path).with_context(|| format!("Failed to read directory at {:?}", path))?;

    for path in paths {
        let file_name = path?.file_name();

        println!(
            "{}",
            file_name
                .to_str()
                .ok_or_else(|| anyhow!("Non UTF-8 file name"))?
        );
    }
}
