use fehler::throws;
use std::fs::{self, File};
use std::io::Write;

use crate::config::{self, Config};
use crate::paths::GeneralPaths;

#[throws(anyhow::Error)]
pub fn read_cache(gen_paths: &GeneralPaths) -> Config {
    let contents = fs::read_to_string(&gen_paths.cache_file)?;

    bincode::deserialize(contents.as_bytes())?
}

#[throws(anyhow::Error)]
pub fn write_cache(gen_paths: &GeneralPaths, config: &Config) {
    let bytes = bincode::serialize(config)?;

    let mut cache_file = File::create(&gen_paths.cache_file)?;

    cache_file.write_all(&bytes)?;
}

#[throws(anyhow::Error)]
pub fn update_cache(gen_paths: &GeneralPaths) -> Config {
    let config_toml = config::read_config(&gen_paths)?;
    let config = config::parse_config_toml(gen_paths, config_toml);
    write_cache(&gen_paths, &config)?;

    config
}
