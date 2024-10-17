#pragma once
#include <glm/gtc/matrix_transform.hpp>

struct Camera
{
    glm::vec3 position{0.0f,0.0f, -3.0f};
    glm::vec3 front{0.0f,0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    float yaw{-90.f};
    float pitch{0.f};
};

//@Memory: why not update in place?
// this is some messed up construction that we will fix later.
inline Camera update_camera(
	const Camera& old_camera,
	float dt,
	bool w_pressed,
	bool a_pressed,
	bool s_pressed,
	bool d_pressed,
    float move_speed)
{
	Camera camera = old_camera;

    float velocity = move_speed * dt;

    if (w_pressed) camera.position += camera.front * velocity;  // Move forward
    if (s_pressed) camera.position -= camera.front * velocity;  // Move backward
    if (a_pressed) camera.position -= glm::normalize(glm::cross(camera.front, camera.up)) * velocity;  // Move left
    if (d_pressed) camera.position += glm::normalize(glm::cross(camera.front, camera.up)) * velocity;  // Move right

    return camera;
}

//@Memory: why not update in place?
inline Camera look_around(const Camera& old_camera, float x_offset, float y_offset, float mouse_sensitivity)
{
	Camera camera = old_camera;

    x_offset *= mouse_sensitivity;
    y_offset *= mouse_sensitivity;

    camera.yaw += x_offset;
    camera.pitch -= y_offset;

    // Limit pitch angle
    if (camera.pitch > 89.0f) camera.pitch = 89.0f;
    if (camera.pitch < -89.0f) camera.pitch = -89.0f;

    glm::vec3 front{};
    front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    front.y = sin(glm::radians(camera.pitch));
    front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
    camera.front = glm::normalize(front);

    return camera;
}

// Get view matrix from the camera
inline glm::mat4 get_view_matrix(const Camera& camera)
{
    return glm::lookAt(camera.position, camera.position + camera.front, camera.up);
}
