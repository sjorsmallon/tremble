#include "ecs.hpp"
#include "globals.hpp"
#include <cassert>
#include <thread>
#include "server_config.hpp"
#include "udp_socket.h"
#include "message.h"
#include <vector>
#include <string>


using namespace std::literals;

struct vec3f_t
{
    union {
        struct {
            float x,y,z;
        };
        struct {
            float a,b,c;
        };
        struct {
            float u,v,w;
        };
        float arr[3];
      };
};

struct Position
{
    float x;
    float y;
    float z;
};

// where are they looking?
struct Orientation
{
    vec3f_t forward;
    vec3f_t right;
};

// where are they heading?
struct Velocity
{
    vec3f_t v;
};

static_assert(sizeof(Position) == sizeof(vec3f_t));

// to be synced over the network. this could just be a bag of entities
// but I want to keep players separate for now so I can think about it.
struct GameState
{
    std::vector<Entity> players;
    std::vector<Entity> everything_else;
};

static GameState g_game_state{};

// already this is not good because of the registry thing. we should pass registry in here.
static Entity create_player()
{   
    auto& registry = get_registry();
    auto entity = registry.CreateEntity();
    entity.AddComponent<Position>(10.0f, 10.0f, 0.0f);
    entity.AddComponent<Orientation>(vec3f_t{1.0f, 0.0f, 0.0f},vec3f_t{0.0f, 0.0f, 1.0f});
    entity.AddComponent<Velocity>();

    g_game_state.players.push_back(entity);
    return entity;
}

static bool connect_to_server()
{
    return false;
}

int main(int argc, char* argv[])
{

    UDPsocket client{};
    client.open();
    client.broadcast(true);


    std::vector<std::string> player_names{
        {"Zero"},
        {"One"},
        {"Two"},
        {"Three"}
    };

    // message to send:
    // Join_Server_Message message{};
    // std::copy(player_names[0].begin(), player_names[0].end(), std::begin(message.player_name));  




    if (client.send("HELLO"s, UDPsocket::IPv4::Broadcast(global::port_number)) < 0)
    {
        fprintf(stderr, "send(): failed (REQ)\n");
    }
    else 
    {
        fprintf(stderr, "send message succeeded.\n");
    }
        // std::this_thread::sleep_for(global::iteration_duration);
    
    client.close();

}