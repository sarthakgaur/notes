use fehler::throws;
use std::fs::create_dir_all;

mod cache;
mod clap_app;
mod config;
mod note;
mod paths;
mod request;
mod utils;

use config::Config;
use paths::{GeneralPaths, NotePaths};
use request::Request;

#[throws(anyhow::Error)]
fn main() {
    let matches = clap_app::app().get_matches();

    let request = Request::new(&matches);
    let gen_paths = GeneralPaths::new()?;

    create_dir_all(&gen_paths.cache_dir)?;
    create_dir_all(&gen_paths.config_dir)?;

    let config = Config::new(&gen_paths)?;
    let note_paths = NotePaths::new(&request, &config);

    // This will also create the notes directory.
    create_dir_all(&note_paths.templates_dir)?;

    request.handle(&note_paths)?;
}
