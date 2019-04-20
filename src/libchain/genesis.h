#pragma once
#include "cltype.h"
#include "block.h"


int genesis_load_json(const std::string& path, block_t& b);
int genesis_chain_setting(const std::string& jsonstr, chain_setting_t& set);

