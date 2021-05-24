use std::fs::{self, File};
use std::io::Write;
use std::process;

use crate::config::{self, Config};
use crate::paths::GeneralPaths;

pub fn read_cache(gen_paths: &GeneralPaths) -> Config {
    let contents = fs::read_to_string(&gen_paths.cache_file).unwrap();

    bincode::deserialize(contents.as_bytes()).unwrap()
}

pub fn write_cache(gen_paths: &GeneralPaths, config: &Config) {
    let bytes = bincode::serialize(config).unwrap();

    let mut cache_file = File::create(&gen_paths.cache_file).unwrap();

    if cache_file.write_all(&bytes).is_err() {
        eprintln!("Could not write to the cache file. Exiting...");
        process::exit(1);
    }
}

pub fn update_cache(gen_paths: &GeneralPaths) -> Config {
    let config_toml = config::read_config(&gen_paths);
    let config = config::parse_config_toml(gen_paths, config_toml);
    write_cache(&gen_paths, &config);
    config
}
