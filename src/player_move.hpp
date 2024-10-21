#pragma once


struct Move_Input 
{
	bool forward_pressed;
	bool backward_pressed;
	bool left_pressed;
	bool right_pressed;
	bool jump_pressed;
};

// velocity is a vector. i.e. velocity = {vx, vy, vz}. both magnitude and direction.
// SPEED is how we describe the magnitude of velocity.


// wish_direction is normalized, movement_vector is not.
[[nodiscard]]
vec3 accelerate(vec3 movement_vector, vec3 wish_direction, float wish_speed, float acceleration, float dt)
{
	// how hard are we already moving in that direction?
	// what is the delta we need to add in order to achieve the wish_speed?
    float current_speed_in_wish_direction = dot(movement_vector, wish_direction);
    float add_speed = wish_speed - current_speed_in_wish_direction;

    // actually, we are going too fast?
    if (add_speed < 0.0f) return movement_vector;

    float acceleration_speed = acceleration * dt * wish_speed;
    // do we not overshoot the wish_speed?
    if (acceleration_speed > add_speed) acceleration_speed = add_speed;

    vec3 result = movement_vector + (acceleration_speed * wish_direction);

    return result;
}   

[[nodiscard]]
std::tuple<vec3, vec3> step_slide_move(const vec3& old_position, const vec3& movement_vector, const bool player_is_airborne, const float dt)
{
	constexpr auto g_gravity = 800.f;
    vec3 new_movement_vector = movement_vector;

    if (player_is_airborne)
    {
        new_movement_vector.y -= g_gravity * dt;
    }

    vec3 position = old_position + (new_movement_vector * dt);
    std::print("new_position: {}\n", position);
    return std::make_tuple(position, new_movement_vector);
}


auto apply_friction(vec3 old_movement_vector, bool grounded, bool jump_pressed_this_frame, float dt) -> vec3
{
	//@Hardcode:
	auto pm_stopspeed = 100.0f;
	auto pm_friction = 6.0f;

	// snap to only planar movement.
    if (grounded) old_movement_vector.y = 0.0f;

    float speed = length(old_movement_vector);
    //@Hardcode: what should this be?
    // if we are very small moving, instead of infinitely applying drag, just snap stop.
    const float speed_treshold = 1.0f;
    if (speed < speed_treshold)
    {
        return vec3{};
    }

    float speed_drop = 0.0f;
    if (grounded && !jump_pressed_this_frame) // do not induce a speed drop if we are intending to jump this frame.
    {
        float control = speed < pm_stopspeed ? pm_stopspeed : speed;
        speed_drop += control * pm_friction * dt;
    }

    // adjust the speed with the induced speed drop. 
    float adjusted_speed = speed - speed_drop;

    // cannot move in the negatives.
    if(adjusted_speed < 0.0f) adjusted_speed = 0.0f;

    // normalize the velocity based on the ground velocity (why?) -> how much faster or slower are we going w.r.t the original vector.
    // we could also just normalize the old_movement_vector and multiply it by the adjusted_speed without the need for dividing adjusted_speed by 'speed'.
    if(adjusted_speed > 0.0f) adjusted_speed /= speed;

    return old_movement_vector * adjusted_speed;
}

// since input can be provided -127 -> +127, scale the movement vector based on the input delivered.
[[nodiscard]]
float calculate_input_scale(const float forward_move,const float right_move, const float up_move, const float max_velocity, const float input_axial_extreme)
{

    int max = abs(static_cast<int>(forward_move));
    if (abs(static_cast<int>(right_move)) > max) max = abs(static_cast<int>(right_move));

    if (abs(static_cast<int>(up_move)) > max) max = abs(static_cast<int>(up_move));

    if (!max) return 0.f;

    float total = sqrt(forward_move * forward_move + right_move * right_move + up_move * up_move);
    float scale = max_velocity * static_cast<float>(max) / (input_axial_extreme * total);
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


// what are we actually returning?
// new position, new_movement_vector.
std::tuple<vec3, vec3> walk_move(
	Move_Input& input,
    const vec3 old_position,
    const vec3 old_movement_vector,
    const vec3 front,
    const vec3 right,
    const float dt)
{
	static bool grounded = true; // uh.. why is this not part of player state?


	constexpr auto pm_input_axial_extreme = 127.0f;	
	constexpr auto pm_maxspeed = 320.f; 
	constexpr auto pm_ground_acceleration = 10.f; 
	constexpr auto pm_air_acceleration = 2.f;
	constexpr auto pm_overbounce = 1.001f;

    bool jump_pressed_this_frame = check_jump(input);

    if (jump_pressed_this_frame) grounded = false;

    // apply friction (come to a standstill if no input is applied.)
    vec3 movement_vector_with_friction_applied = apply_friction(old_movement_vector, grounded, jump_pressed_this_frame, dt);

    float forward_input = pm_input_axial_extreme * input.forward_pressed - pm_input_axial_extreme * input.backward_pressed;
    float right_input   = pm_input_axial_extreme * input.right_pressed   - pm_input_axial_extreme * input.left_pressed;
    float up_input      = pm_input_axial_extreme * (jump_pressed_this_frame && grounded);

    // input scale is dependent on axial input. Keyboard keys always provide maximum input
    // i.e. "how much of the direction vector should we apply, since we can tilt a bit instead of all at once."
    // the direction vector is already scaled "to the max", i.e. multiplied by the axial extreme.
    // axial extreme is the same for all axes now.
    float scale = calculate_input_scale(forward_input, right_input, up_input, pm_maxspeed, pm_input_axial_extreme);

    // pull out y component.
    vec3 plane_front = vec3{front.x, 0.0f, front.z};
    vec3 plane_right = vec3{right.x, 0.0f, right.z};

    // does this even do anything? since we pulled out the y component already? or was this different in the past?
    // slide along the ground plane 
    vec3 new_plane_front = clip_vector(plane_front, vec3{0.0f,1.0f,0.0f}, pm_overbounce);
    vec3 new_plane_right = clip_vector(plane_right, vec3{0.0f,1.0f,0.0f}, pm_overbounce);
	plane_front = new_plane_front;
	plane_right = new_plane_right;


    plane_front = normalize(plane_front);
    plane_right = normalize(plane_right);



    // what is the resulting direction we should take, based on the direction I was heading in,
    // and the new addition that we applied? plane_front and plane_right are normalized here.
    // where does the speed go?
    vec3 wish_direction = plane_front * forward_input + plane_right * right_input;
    float wish_speed = 0.0f;

    bool received_input = (input.forward_pressed  ||
                           input.backward_pressed ||
                           input.left_pressed     ||
                           input.right_pressed);

    // normalize input vector, but keep velocity! this HAS to be normalized by dt later.
    if (received_input)
    {
        wish_speed  = scale * length(wish_direction);
        wish_direction = normalize(wish_direction); 
    } 

    // from this point onwards, wish_direction is normalized.

    // if we are ducking, reduce the wishspeed by 0.5.
    
    vec3 movement_vector = vec3{};

    if (wish_speed < 0.1f) 
    {
        movement_vector = movement_vector_with_friction_applied;
    }
    else
    {
        // Take into account whether we need air value or not.
        float acceleration = (grounded) ? pm_ground_acceleration : pm_air_acceleration;
        movement_vector = accelerate(movement_vector_with_friction_applied, wish_direction, wish_speed, acceleration, dt);
    }

    // slide along the ground plane.
    // clip the movement vector to not move into the ground. note that we take the velocity before we do floor clipping.
    // this means we retain the speed we had, just now in a different direction.
    float velocity  = length(movement_vector);
    movement_vector = clip_vector(movement_vector, vec3{0.0f,1.0f,0.0f}, pm_overbounce);

    float movement_treshold = 0.05f;

    //@TEMPORARY!!!!!! the normalize below causes NaN if movement_vector is zero.
    if (fabs(movement_vector.x) < movement_treshold && fabs(movement_vector.z) < movement_treshold)
    {
        return std::make_tuple(old_position, vec3{0.0f, movement_vector.y, 0.0f});
    }

    // don't decrease velocity when going down a slope.
    movement_vector = normalize(movement_vector);
    movement_vector = velocity * movement_vector;

    // skip checking the movement vector if the movement vector is too small.
    if (fabs(movement_vector.x) < movement_treshold && fabs(movement_vector.z) < movement_treshold)
    {
        return std::make_tuple(old_position, vec3{0.0f, movement_vector.y, 0.0f});
    }

    bool player_is_airborne = false;;

    return step_slide_move(old_position, movement_vector, player_is_airborne, dt);
}

// it is too complex for me to think about. retry.
// new position, new_movement_vector.
// this function does the following.
// 1. apply friction to the existing velocity.
// 2. clip the front / right vector according to the ground face normal.
// 3. devise a "wish direction": what direction do the keys tell me to go in?
// 3. if we actually provided enough input (over some treshold), accelerate "along" the wish direction.
//
 
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
	constexpr auto pm_air_acceleration = 2.f;
	constexpr auto pm_overbounce = 1.001f;

    bool jump_pressed_this_frame = check_jump(input);

    if (jump_pressed_this_frame) grounded = false;

    // apply friction (come to a standstill if no input is applied.)
    vec3 old_velocity_with_friction_applied = apply_friction(old_velocity, grounded, jump_pressed_this_frame, dt);

    // what inputs did we provide?
    float forward_input = pm_input_axial_extreme * input.forward_pressed - pm_input_axial_extreme * input.backward_pressed;
    float right_input   = pm_input_axial_extreme * input.right_pressed   - pm_input_axial_extreme * input.left_pressed;
    float up_input      = pm_input_axial_extreme * (jump_pressed_this_frame && grounded);

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
    float wish_speed = 0.0f; // I think because of some float weirdness that taking the length of wish_direction when it is 0 does something weird.

    if (received_input)
    {
    	// how hard did we move the joystick? -127... +127. button presses are always max (127) or min (127).
    	// this makes the "wish" direction (the one purely based on input) less strong.

	    float scale = calculate_input_scale(forward_input, right_input, up_input, pm_maxspeed, pm_input_axial_extreme);
        wish_speed  = scale * length(wish_direction);
    } 

    vec3 movement_vector{};


    // I understand that "just" adding the vectors will yield something awful. but what does acceleration actually mean in this context?
    // the old_velocity vector is just the true "speed" we have.
    // what are the units here?
    if (wish_speed < 0.1f) 
    {
        movement_vector = old_velocity_with_friction_applied;
    }
    else
    {
        // Take into account whether we need air value or not.
        float acceleration = (grounded) ? pm_ground_acceleration : pm_air_acceleration;
        movement_vector = accelerate(old_velocity_with_friction_applied, wish_direction, wish_speed, acceleration, dt);
    }


}