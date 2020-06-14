#ifndef CACHE_H
#define CACHE_H

int cache_controller(struct config *conf, char *home_str, char *config_path, char *cache_path);
void write_cache(struct config *conf, char *cache_path);

#endif
