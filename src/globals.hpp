#include "ecs.hpp"

Registry& get_registry()
{
    static Registry registry{}; // Guaranteed to be destroyed.
                            // Instantiated on first use.
    return registry;
}