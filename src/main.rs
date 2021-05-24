mod cache;
mod clap_app;
mod config;
mod note;
mod paths;
mod request;
mod utils;

fn main() {
    let matches = clap_app::app().get_matches();

    let request = request::Request::new(&matches);
    let gen_paths = paths::build_gen_paths();

    utils::create_dir(&gen_paths.cache_dir);
    utils::create_dir(&gen_paths.config_dir);

    let config = config::build_config(&gen_paths);
    let note_paths = paths::build_note_paths(&request, &config);

    // This will also create the notes directory.
    utils::create_dir(&note_paths.templates_dir);

    request.handle(&note_paths);
}
