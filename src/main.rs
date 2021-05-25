use fehler::throws;
use std::fs::create_dir_all;

mod cache;
mod clap_app;
mod config;
mod note;
mod paths;
mod request;
mod utils;

#[throws(anyhow::Error)]
fn main() {
    let matches = clap_app::app().get_matches();

    let request = request::Request::new(&matches);
    let gen_paths = paths::build_gen_paths()?;

    create_dir_all(&gen_paths.cache_dir)?;
    create_dir_all(&gen_paths.config_dir)?;

    let config = config::build_config(&gen_paths)?;
    let note_paths = paths::build_note_paths(&request, &config);

    // This will also create the notes directory.
    create_dir_all(&note_paths.templates_dir)?;

    request.handle(&note_paths)?;
}
