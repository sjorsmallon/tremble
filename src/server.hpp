#pragma once


constexpr int MAX_CLIENTS = 64;

struct Server
{
    int max_clients = MAX_CLIENTS;
    int connected_clients_count;
    bool client_connected[MAX_CLIENTS];
    Address client_addresses[MAX_CLIENTS];
};

int find_free_client_idx(Server& server)
{
    for (int idx = 0; idx < server.max_clients; ++idx)
    {
        if (!server.client_connected[idx])
        {
            return idx;
        }
    }
    return -1; // sentinel value.
}

int find_existing_client_idx(Server& server, Address& address)
{
    for (int idx = 0; idx < server.max_clients; ++idx)
    {
        if (server.client_connected[idx] && server.client_addresses[idx] == address)
        {
            return idx;
        }
    }
    return -1; // sentinel value
}

bool is_client_conntect(Server& server, int client_idx)
{
    return server.client_connected[client_idx];
}

Address& Server::get_client_address(Server& server, int client_idx)
{
    return server.client_addresses[client_idx];
}

enum CONNECTION: uint8_t
{
    CLIENT_CONNECTION_REQUEST,
    SERVER_CONNECTION_ACCEPT,
    SERVER_CONNECTION_CHALLENGE,
};

struct Connection_Request
{
    uint8_t message = CLIENT_CONNECTION_REQUEST;
    uint64_t salt;
};

struct Connection_Response
{
    uint8_t message = SERVER_CONNECTION_CHALLENGE,
    uint64_t client_salt;
    uint64_t server_salt;
};

