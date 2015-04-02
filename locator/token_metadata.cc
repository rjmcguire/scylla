/*
 * Copyright (C) 2015 Cloudius Systems, Ltd.
 */

#include "token_metadata.hh"

namespace locator {

token_metadata::token_metadata(std::map<token, inet_address> token_to_endpoint_map, boost::bimap<inet_address, utils::UUID> endpoints_map, topology topology) :
    _token_to_endpoint_map(token_to_endpoint_map), _endpoint_to_host_id_map(endpoints_map), _topology(topology) {
    _sorted_tokens = sorted_tokens();
}

std::vector<token> token_metadata::sort_tokens() {
    std::vector<token> sorted;
    sorted.reserve(_token_to_endpoint_map.size());

    for (auto&& i : _token_to_endpoint_map) {
        sorted.push_back(i.first);
    }

    return sorted;
}

const std::vector<token>& token_metadata::sorted_tokens() const {
    return _sorted_tokens;
}

/**
 * Update token map with a single token/endpoint pair in normal state.
 */
void token_metadata::update_normal_token(token t, inet_address endpoint)
{
    update_normal_tokens(std::unordered_set<token>({t}), endpoint);
}

void token_metadata::update_normal_tokens(std::unordered_set<token> tokens, inet_address endpoint) {
    std::unordered_map<inet_address, std::unordered_set<token>> endpoint_tokens ({{endpoint, tokens}});
    update_normal_tokens(endpoint_tokens);
}

/**
 * Update token map with a set of token/endpoint pairs in normal state.
 *
 * Prefer this whenever there are multiple pairs to update, as each update (whether a single or multiple)
 * is expensive (CASSANDRA-3831).
 *
 * @param endpointTokens
 */
void token_metadata::update_normal_tokens(std::unordered_map<inet_address, std::unordered_set<token>>& endpoint_tokens) {
    if (endpoint_tokens.empty()) {
        return;
    }

    bool should_sort_tokens = false;
    for (auto&& i : endpoint_tokens)
    {
        inet_address endpoint = i.first;
        std::unordered_set<token>& tokens = i.second;

        assert(!tokens.empty());

        for(auto it = _token_to_endpoint_map.begin(), ite = _token_to_endpoint_map.end(); it != ite;) {
            if(it->second == endpoint) {
                it = _token_to_endpoint_map.erase(it);
            } else {
                ++it;
            }
        }

#if 0
        bootstrapTokens.removeValue(endpoint);
        topology.addEndpoint(endpoint);
        leavingEndpoints.remove(endpoint);
        removeFromMoving(endpoint); // also removing this endpoint from moving
#endif
        for (const token& t : tokens)
        {
            auto prev = _token_to_endpoint_map.insert(std::pair<token, inet_address>(t, endpoint));
            should_sort_tokens |= prev.second; // new token inserted -> sort
            if (prev.first->second  != endpoint) {
                //logger.warn("Token {} changing ownership from {} to {}", token, prev.first->second, endpoint);
                prev.first->second = endpoint;
            }
        }
    }

    if (should_sort_tokens) {
        _sorted_tokens = sort_tokens();
    }
}

size_t token_metadata::first_token_index(const token& start) {
    assert(_sorted_tokens.size() > 0);
    auto it = std::lower_bound(_sorted_tokens.begin(), _sorted_tokens.end(), start);
    if (it == _sorted_tokens.end()) {
        return 0;
    } else {
        return std::distance(_sorted_tokens.begin(), it);
    }
}

const token& token_metadata::first_token(const token& start) {
    return _sorted_tokens[first_token_index(start)];
}

inet_address token_metadata::get_endpoint(const token& token) const {
    auto it = _token_to_endpoint_map.find(token);
    assert (it != _token_to_endpoint_map.end());
    return it->second;
}
}
