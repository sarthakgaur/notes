use std::fs::{self, OpenOptions};
use std::io::Write;
use std::process;

use crate::config::{self, Config};
use crate::paths::GeneralPaths;

pub fn read_cache(gen_paths: &GeneralPaths) -> Config {
    let contents = fs::read_to_string(&gen_paths.cache_file).unwrap();
    let bytes = contents.as_bytes();
    let config: Config = bincode::deserialize(bytes).unwrap();
    return config;
}

pub fn write_cache(gen_paths: &GeneralPaths, config: &Config) {
    let bytes = bincode::serialize(config).unwrap();

    let mut cache_file = OpenOptions::new()
        .write(true)
        .truncate(true)
        .open(&gen_paths.cache_file)
        .unwrap();

    if let Err(_) = cache_file.write_all(&bytes) {
        eprintln!("Could not write to the cache file. Exiting...");
        process::exit(1);
    }
}

pub fn update_cache(gen_paths: &GeneralPaths) -> Config {
    let config_toml = config::read_config(&gen_paths);
    let config = config::parse_config_toml(gen_paths, config_toml);
    write_cache(&gen_paths, &config);
    return config;
}
