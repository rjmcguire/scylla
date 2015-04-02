/*
 * Copyright (C) 2015 Cloudius Systems, Ltd.
 */

#include <algorithm>
#include "simple_strategy.hh"

namespace locator {

simple_strategy::simple_strategy(const sstring& keyspace_name, token_metadata& token_metadata, std::unordered_map<sstring, sstring>& config_options) :
        abstract_replication_strategy(keyspace_name, token_metadata, config_options) {}

std::vector<inet_address> simple_strategy::calculate_natural_endpoints(const token& t) {
    size_t replicas = get_replication_factor();
    const std::vector<token>& tokens = _token_metadata.sorted_tokens();
    std::vector<inet_address> endpoints;
    endpoints.reserve(replicas);

    if (tokens.empty()) {
        return endpoints;
    }

    auto it = tokens.begin() + _token_metadata.first_token_index(t);
    auto c = tokens.size();

    while(endpoints.size() < replicas && c) {
        inet_address ep = _token_metadata.get_endpoint(*(it++));
        if (std::find(endpoints.begin(), endpoints.end(), ep) == endpoints.end()) {
            endpoints.push_back(ep);
        }
        c--;
        // wrap around
        if (it == tokens.end()) {
            it = tokens.begin();
        }
    }
    return endpoints;
}

size_t simple_strategy::get_replication_factor() const {
    auto it = _config_options.find("replication_factor");
    if (it == _config_options.end()) {
        return 1;
    }
    return std::stol(it->second);
}

static replication_strategy_registrator registerator("org.apache.cassandra.locator.SimpleStrategy",
        [] (const sstring& keyspace_name, token_metadata& token_metadata, std::unordered_map<sstring, sstring>& config_options) {
   return std::make_unique<simple_strategy>(keyspace_name, token_metadata, config_options);
});

}
