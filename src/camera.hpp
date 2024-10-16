#pragma once
#include <glm/gtc/matrix_transform.hpp>

struct camera
{
    glm::vec3 position{0.0f,0.0f, -3.0f};
    glm::vec3 front{0.0f,0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f}
    float yaw{-90.f};
    float pitch{0.f};
};

void update_camera(camera& camera, float dt, glm::vec2 input)
{
    float velocity = MOVE_SPEED * dt;

    // if (state[SDL_SCANCODE_W]) camera.position += camera.front * velocity;  // Move forward
    // if (state[SDL_SCANCODE_S]) camera.position -= camera.front * velocity;  // Move backward
    // if (state[SDL_SCANCODE_A]) camera.position -= glm::normalize(glm::cross(camera.front, camera.up)) * velocity;  // Move left
    // if (state[SDL_SCANCODE_D]) camera.position += glm::normalize(glm::cross(camera.front, camera.up)) * velocity;  // Move right
}