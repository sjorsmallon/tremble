#pragma once
#include <print>


//@Note: this move input is serialized and sent across the wire. I don't think that this is the correct place to define it.
// but I will leave it here for now.
struct Move_Input 
{
	bool forward_pressed;
	bool backward_pressed;
	bool left_pressed;
	bool right_pressed;
	bool jump_pressed;
};

Move_Input generate_random_input()
{
    Move_Input input{};

    // Assign random boolean values to each field (true or false)
    input.forward_pressed = rand() % 2;   // 0 or 1
    input.backward_pressed = rand() % 2;  // 0 or 1
    input.left_pressed = rand() % 2;      // 0 or 1
    input.right_pressed = rand() % 2;     // 0 or 1
    // input.jump_pressed = rand() % 2;      // 0 or 1

    return input;   
}


// Velocity is a vector. i.e. velocity = {vx, vy, vz}. both magnitude and direction.
// SPEED is how we describe the magnitude of velocity.




// wish_direction is normalized, new_velocity is not.
[[nodiscard]]
vec3 accelerate(vec3 new_velocity, vec3 wish_direction, float wish_speed, float acceleration, float dt)
{
	// how hard are we already moving in that direction?
	// what is the delta we need to add in order to achieve the wish_speed?
    float current_speed_in_wish_direction = dot(new_velocity, wish_direction);
    float add_speed = wish_speed - current_speed_in_wish_direction; 

    //  current_speed_in_wish_direction exceeds the wish_speed in that direction, that's fine. no need to do anything.
    if (add_speed < 0.0f) return new_velocity;

    float acceleration_speed = acceleration * dt * wish_speed;

    // even if we overshoot the wish speed, it will be clipped in step_slide_move.
    if (acceleration_speed > add_speed) acceleration_speed = add_speed;

    vec3 result = new_velocity + (acceleration_speed * wish_direction);

    return result;
}   

[[nodiscard]]
std::tuple<vec3, vec3> step_slide_move(const vec3& old_position, vec3& new_velocity, const bool player_is_airborne, const float dt)
{
    constexpr auto pm_maxspeed = 320.f; //@VOLATILE: also in my move.
	constexpr auto g_gravity = 800.f;

    if (player_is_airborne)
    {
        new_velocity.y -= g_gravity * dt;
    }
    // collide with the ground.

    // clip the speed in the horizontal plane to maxspeed.
    float speed = sqrt(new_velocity.x * new_velocity.x + new_velocity.z * new_velocity.z);
    if (speed > pm_maxspeed)
    {
        speed = pm_maxspeed;
        float old_y = new_velocity.y;

        auto new_vector = vec3{new_velocity.x, 0.0f, new_velocity.z};
        new_vector = normalize(new_vector);
        new_vector = new_vector * speed;
        new_velocity = vec3{new_vector.x, old_y, new_vector.z};
    }


    vec3 position = old_position + (new_velocity * dt);

    // test if we can actually be at the new position (collide with the environment and push back). for now, we take this to be y = 10.f;
    if (position.y < 10.0f)
    {
        std::print("snapping to floor.\n");
        position.y = 10.f;
        new_velocity.y = 0.f;
    }


    return std::make_tuple(position, new_velocity);
}


auto apply_friction(vec3 old_velocity, bool grounded, bool jump_pressed_this_frame, float dt) -> vec3
{
	//@Hardcode:
	auto pm_stopspeed = 100.0f;
	auto pm_friction = 6.0f;

	// snap to only planar movement.
    if (grounded) old_velocity.y = 0.0f;

    float speed = length(old_velocity);
    //@Hardcode: what should this be?
    // if we are very small moving, instead of infinitely applying drag, just snap stop.
    const float speed_treshold = 1.0f;
    if (speed < speed_treshold)
    {
        return vec3{};
    }

    float speed_drop = 0.0f;
    if (grounded && !jump_pressed_this_frame) // do not induce a speed drop if we are intending to jump this frame. (and do not induce a speed drop if we are flying.)
    {
        float control = speed < pm_stopspeed ? pm_stopspeed : speed;
        speed_drop += control * pm_friction * dt;
    }

    // adjust the speed with the induced speed drop. 
    float adjusted_speed = speed - speed_drop;

    // cannot move in the negatives.
    if(adjusted_speed < 0.0f) adjusted_speed = 0.0f;

    // normalize the velocity based on the ground velocity (why?) -> how much faster or slower are we going w.r.t the original vector.
    // we could also just normalize the old_velocity and multiply it by the adjusted_speed without the need for dividing adjusted_speed by 'speed'.
    if(adjusted_speed > 0.0f) adjusted_speed /= speed;

    return old_velocity * adjusted_speed;
}

// since input can be provided -127 -> +127, scale the movement vector based on the input delivered.
[[nodiscard]]
float calculate_input_scale(const float forward_move,const float right_move, const float up_move, const float max_speed, const float input_axial_extreme)
{

    int max = abs(static_cast<int>(forward_move));
    if (abs(static_cast<int>(right_move)) > max) max = abs(static_cast<int>(right_move));

    if (abs(static_cast<int>(up_move)) > max) max = abs(static_cast<int>(up_move));

    if (!max) return 0.f;

    float total = sqrt(forward_move * forward_move + right_move * right_move + up_move * up_move);
    float scale = max_speed * static_cast<float>(max) / (input_axial_extreme * total);
    return scale;
}

inline bool check_jump(const Move_Input& input)
{
	return input.jump_pressed;
}


inline vec3 clip_vector(vec3 in, vec3 normal, const float overbounce)
{
	// how strong is the incoming vector in the direction of the face normal? (i.e. we split the incoming vector in two parts: the one that is parallel to the normal,
	// and the one that is perpendicular to it (along the wall).
    float backoff = dot(in, normal);

    if (backoff < 0.0f)
    {
        backoff *= overbounce;
    }
    else
    {
        backoff /= overbounce;
    }

    vec3 change = normal * backoff;

    // in effect: remove the part of the vector that is heading into the wall. just keep the part that was perpendicular.
    vec3 result = in - change;

    return result;
}

// it is too complex for me to think about. retry.
// new position, new_new_velocity.
std::tuple<vec3, vec3> my_walk_move(
	Move_Input& input,
    const vec3 old_position,
    const vec3 old_velocity,
    const vec3 front,
    const vec3 right,
    const float dt)
{
	static bool grounded = true;

	constexpr auto pm_input_axial_extreme = 127.0f;	
	constexpr auto pm_maxspeed = 320.f; 
	constexpr auto pm_ground_acceleration = 10.f; 
	constexpr auto pm_air_acceleration = 5.0f;
	constexpr auto pm_overbounce = 1.001f;
    constexpr auto pm_movement_treshold = 0.00001f; //minimum necessary movement in either axis.
    constexpr auto pm_jumpspeed = 270.f;
    constexpr  auto g_gravity = 800.f; //@VOLATILE: also fix in slide_move.

    bool jump_pressed_this_frame = check_jump(input);
    if (jump_pressed_this_frame)
    {
        grounded = false;
    }

    // apply friction. this does not fully 'nullify' the velocity (or does it?). 
    vec3 old_velocity_with_friction_applied = apply_friction(old_velocity, grounded, jump_pressed_this_frame, dt);

    // what inputs did we provide?
    float forward_input = pm_input_axial_extreme * input.forward_pressed - pm_input_axial_extreme * input.backward_pressed;
    float right_input   = pm_input_axial_extreme * input.right_pressed   - pm_input_axial_extreme * input.left_pressed;
    float up_input      = pm_input_axial_extreme * (jump_pressed_this_frame && !grounded);

    // get rid of the y component: only look at the xz plane. the y-component is handled by "a different subroutine".
    // where are we looking?
    vec3 front_without_y = vec3{front.x, 0.0f, front.z};
    vec3 right_without_y = vec3{right.x, 0.0f, right.z};

    // look at the floor below you. this is known as a "ground trace". what is the normal of that face?
    // imagine it is steep, like an incline. we do not want to move inside of that, but move smoothly
    // perpendicular to that normal. so we "clip" the velocity vector such that we redirect it along that perpendicular axis.
	// for now, no inclines. just flat surfaces. so the normal is always {0.0f, 1.0f, 0.0f};
	vec3 ground_face_normal{0.0f,1.0f, 0.0f};

	vec3 front_clipped = clip_vector(front_without_y, ground_face_normal, pm_overbounce);
	vec3 right_clipped = clip_vector(right_without_y, ground_face_normal, pm_overbounce);



 	bool received_input = (input.forward_pressed  ||
                           input.backward_pressed ||
                           input.left_pressed     ||
                           input.right_pressed);


	// what is the resulting direction we should take, based on the new clipped front and right (accounting for the walls we might be colliding with),
	// and what buttons I pressed in relation to those vectors.
    vec3 wish_direction = front_clipped * forward_input + right_clipped * right_input;
    vec3 normalized_wish_direction = normalize(wish_direction);


    float input_scale = calculate_input_scale(forward_input, right_input, up_input, pm_maxspeed, pm_input_axial_extreme);
    float wish_speed = 0.0f; // I think because of some float weirdness that taking the length of wish_direction when it is 0 does something weird.

    if (received_input)
    {
    	// how hard did we move the joystick? -127... +127. button presses are always max (127) or min (127).
    	// this makes the "wish" direction (the one purely based on input) less strong.
        wish_speed  = input_scale * length(wish_direction);
    } 

    vec3 new_velocity{};

    if (wish_speed < 0.0000001f) 
    {
        new_velocity = old_velocity_with_friction_applied;
    }
    else
    {
        // if we are in the air, you have less control.
        float acceleration = (grounded) ? pm_ground_acceleration : pm_air_acceleration;
        new_velocity = accelerate(old_velocity_with_friction_applied, normalized_wish_direction, wish_speed, acceleration, dt);
    }

    // clip the new velocity it against the ground plane. take the length before it is clipped.
    float new_speed  = length(new_velocity);
    new_velocity = clip_vector(new_velocity, ground_face_normal, pm_overbounce);
    

    bool player_is_airborne = (old_position.y > 10.0f);
    if (!player_is_airborne) grounded = true;

    //@Note: this is disabled for now because it apparently does not matter (yet) in the new implementation.
    // if we start seeing nan's, i'll re-enable it. - Sjors, 22-10-2024

    // not moving enough? early exit. set x and z velocity to zero, keep y in case the player is falling.
    // if (fabs(new_velocity.x) < pm_movement_treshold && fabs(new_velocity.z) < pm_movement_treshold)
    // {
    //     if (jump_pressed_this_frame && !player_is_airborne)
    //     {

    //         new_velocity.y = (pm_jumpspeed * input_scale);
    //     }else if (player_is_airborne)
    //     {
    //         new_velocity.y = old_velocity.y;
    //         new_velocity.y -= g_gravity * dt;
    //     }

    //     new_velocity = vec3{0.0f, new_velocity.y, 0.0f};
    //     vec3 new_position = old_position + new_velocity * dt;
    //     return std::make_tuple(new_position, new_velocity);
    // }


    // since we take the velocity before clipping. it can be we clip the movement vectors (effectively reducing player speed.)
    // but we still want to retain the speed we were moving in before.
    new_velocity = normalize(new_velocity);
    new_velocity = new_speed * new_velocity;


    //@Note: this is disabled for now because it apparently does not matter (yet) in the new implementation.
    // if we start seeing nan's, i'll re-enable it. - Sjors, 22-10-2024


    // skip checking the movement vector if the movement vector is too small.
    // if (fabs(new_velocity.x) < pm_movement_treshold && fabs(new_velocity.z) < pm_movement_treshold)
    // {
    //     if (jump_pressed_this_frame && !player_is_airborne)
    //     {
    //         new_velocity.y = (pm_jumpspeed * input_scale);
    //     }else if (player_is_airborne)
    //     {
    //         new_velocity.y = old_velocity.y;
    //         new_velocity.y -= g_gravity * dt;
    //     }
        
    //     new_velocity = vec3{0.0f, new_velocity.y, 0.0f};
    //     vec3 new_position = old_position + new_velocity * dt;
    //     return std::make_tuple(new_position, new_velocity);
    // }

    // if the player is not airborne, return grounded to true. 
    if (!player_is_airborne)
    {
        grounded = true;
    }


    // just before step_slide-move, reassign the y velocity (that has not been touched up until now.)
    new_velocity.y = old_velocity.y;

    // we are missing where to inject the jump. so let me just do that here.
    // a jump is not a velocity, but just a "set speed" for one particular frame, that
    // gets removed over time with gravity.
    if (jump_pressed_this_frame && !player_is_airborne)
    {
        new_velocity.y = (pm_jumpspeed * input_scale);
    }

    return step_slide_move(old_position, new_velocity, player_is_airborne, dt);
}