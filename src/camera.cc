#include "core/engine_settings.hh"
#include "core/camera.hh"
#include "core/engine.hh"
#include "core/window.hh"
#include "core/tools.hh"
#include "core/input.hh"
#include "core/sound.hh"

namespace kretash {

  Camera::Camera() :
    m_aspect_ratio( 0.0f ),
    m_fov( 0.0f ),
    m_znear( 0.0f ),
    m_zfar( 0.0f ),
    m_cinematic_camera( false ),
    m_barrel_rolling( false ),
    m_last_barrel_roll( -100.0f ),
    m_looping_the_loop( false ),
    m_last_loop_the_loop( 0.0f ) {

    m_eye = float3( 15.0f, 2.5f, 15.0f );
    m_look_dir = float3( 1.0f, -0.5f, 1.0f );
    m_move_dir = float3( 1.0f, 0.0f, 1.0f );
    m_focus = m_eye + m_look_dir;

    m_cinematic_camera = k_engine_settings->get_settings().animated_camera;
  }

  void Camera::init() {
    kretash::Window* w = k_engine->get_window();
    init( w->get_aspect_ratio(), to_rad( 80.0f ), 0.1f, 2000.0f );
  }

  void Camera::init( float aspect_ratio, float fov, float znear, float zfar ) {
    using namespace tools;

    m_aspect_ratio = aspect_ratio;
    m_fov = fov;
    m_znear = znear;
    m_zfar = zfar;

    bool vulkan = k_engine_settings->get_settings().m_api == kVulkan;
    if( vulkan )
      m_up = float3( 0.0f, -1.0f, 0.0f );
    else
      m_up = float3( 0.0f, 1.0f, 0.0f );

    m_view = float4x4( 1.0f );
    m_view.look_at( m_eye, m_focus, m_up );

    m_projection = float4x4( 1.0f );
    m_projection.prespective( fov, m_aspect_ratio, znear, zfar );

  }

  void Camera::api_swap() {

    float sensitivity = 0.004f;
    Input* input = k_engine->get_input();

    float mx = sensitivity * ( float ) input->get_cursor().m_x;
    float my = sensitivity * ( float ) input->get_cursor().m_y;

    bool vulkan = k_engine_settings->get_settings().m_api == kVulkan;
    if( vulkan ) {
      m_look_dir.x = sinf( mx );
      m_look_dir.z = cosf( mx );
      m_look_dir.y = tanf( my );
      m_up = float3( 0.0f, -1.0f, 0.0f );
    } else {
      m_look_dir.x = cosf( mx );
      m_look_dir.z = sinf( mx );
      m_look_dir.y = tanf( my );
      m_up = float3( 0.0f, 1.0f, 0.0f );
    }
  }

  void Camera::update() {
    Input* input = k_engine->get_input();
    m_fov = to_rad( k_engine_settings->get_settings().m_base_fov );

    if( m_cinematic_camera ) {
      _cinematic_camera();
    }

    if( input->has_focus() && !m_cinematic_camera ) {
      _controlled_camera();
    }


    if( input->get_key( key::k_F2 ) ) {
      m_cinematic_camera = !m_cinematic_camera;
    }

    m_view = float4x4( 1.0f );
    m_focus = m_eye + m_look_dir;
    m_view.look_at( m_eye, m_focus, m_up );

    m_projection = float4x4( 1.0f );
    m_projection.prespective( m_fov, m_aspect_ratio, m_znear, m_zfar );
  }

  void Camera::_controlled_camera() {

    float mul = k_engine_settings->get_delta_time() / 16.0f;

    Input* input = k_engine->get_input();
    float movement_speed = 1.0f * mul;
    float sensitivity = 0.004f;
    float my_center = 0.0f;
    static float accum_cx = 0.0f;
    static float accum_cy = 0.0f;

    if( input->get_gamepad().buttons & PS4_CONTROLLER_GAMEPAD_LEFT_THUMB ) {
      movement_speed = 8.0f;
    } else if( input->get_key( k_SHIFT ) ) {
      movement_speed = 8.0f;
    }

    float mx = sensitivity * ( float ) input->get_cursor().m_x;
    float my = sensitivity * ( float ) input->get_cursor().m_y;

    accum_cx += -5.0f * sensitivity * ( float ) input->get_gamepad().thumb_R_side;
    accum_cy += 5.0f * sensitivity * ( float ) input->get_gamepad().thumb_R_vert;
    mx += accum_cx;
    my += accum_cy;

    bool vulkan = k_engine_settings->get_settings().m_api == kVulkan;
    if( vulkan ) {
      m_look_dir.x = sinf( mx );
      m_look_dir.z = cosf( mx );
      m_look_dir.y = tanf( my );
    } else {
      m_look_dir.x = cosf( mx );
      m_look_dir.z = sinf( mx );
      m_look_dir.y = tanf( my );
    }

    m_look_dir.normalize();

    if( input->stealth_get_key( k_W ) ) {
      m_eye = m_eye + ( m_look_dir*movement_speed );
    }
    if( input->stealth_get_key( k_S ) ) {
      m_eye = m_eye - ( m_look_dir*movement_speed );
    }
    if( input->stealth_get_key( k_A ) ) {
      float3 strafe = float3( -m_look_dir.z, 0.0f, m_look_dir.x );
      if( vulkan )
        m_eye = m_eye - ( strafe*movement_speed );
      else
        m_eye = m_eye + ( strafe*movement_speed );
    }
    if( input->stealth_get_key( k_D ) ) {
      float3 strafe = float3( -m_look_dir.z, 0.0f, m_look_dir.x );
      if( vulkan )
        m_eye = m_eye + ( strafe*movement_speed );
      else
        m_eye = m_eye - ( strafe*movement_speed );
    }
    if( input->stealth_get_key( k_Q ) ) {
      float3 high = float3( 0.0f, 1.0f, 0.0f );
      m_eye = m_eye - ( high*movement_speed );
    }
    if( input->stealth_get_key( k_E ) ) {
      float3 high = float3( 0.0f, -1.0f, 0.0f );
      m_eye = m_eye - ( high*movement_speed );
    }

    float3 strafe = -float3( -m_look_dir.z, 0.0f, m_look_dir.x );

    if( vulkan )
      m_eye -= ( strafe*movement_speed ) * input->get_gamepad().thumb_L_side;
    else
      m_eye += ( strafe*movement_speed ) * input->get_gamepad().thumb_L_side;

    m_eye += ( m_look_dir*movement_speed ) * input->get_gamepad().thumb_L_vert;
    m_eye += ( float3( 0.0f, -input->get_gamepad().left_trigger, 0.0f )*movement_speed );
    m_eye += ( float3( 0.0f, +input->get_gamepad().right_trigger, 0.0f )*movement_speed );

  }

  void Camera::_cinematic_camera() {

    float time_elapsed = k_engine_settings->get_time_elapsed() * 0.001f;

    std::vector<float> spectrum;
    Sound* sound = k_engine->get_sound();
    sound->get_smooth_simplified_spectrum( &spectrum );

#if 1
    if( !m_looping_the_loop ) {
      m_look_dir = float3( -1.0f,
        -0.2f + sinf( time_elapsed )*spectrum[8],
        0.7f );
      m_look_dir.normalize();
    }

    float base_speed = k_engine_settings->get_settings().anim_camera_base_speed;
    float speed_mult = k_engine_settings->get_settings().anim_camera_music_speed;

    m_eye.z += base_speed * ( cosf( time_elapsed ) + 1.0f ) +
      spectrum[7] * speed_mult +
      spectrum[8] * speed_mult +
      spectrum[9] * speed_mult;

    m_eye.x += base_speed * ( sinf( time_elapsed ) - 1.0f ) +
      spectrum[7] * -speed_mult +
      spectrum[8] * -speed_mult +
      spectrum[9] * -speed_mult;

    m_eye.y = 70.0f +
      base_speed * 10.0f*sinf( time_elapsed ) +
      spectrum[1] * 10.0f +
      spectrum[2] * 10.0f +
      spectrum[3] * 10.0f;
#endif

    _barrel_roll( &spectrum );

#if 0
    _loop_the_loop( &spectrum );

    static float x = 0.0f, y = 0.0f, z = 0.0f;
    x += 0.0f;  y += 0.01f;  z += 0.0f;
    m_up = float3( sinf( x ), sinf( y ), sinf( z ) );
    m_look_dir.y = sinf( y );
    m_up = float3( cosf( y ), cosf( y ), cosf( y ) );
    m_up.normalize();
    m_view.rotate_x( ( sinf( y ) + 1.0f )*0.01f );
    m_view.rotate_z( ( sinf( y ) + 1.0f )*0.01f );
#endif

    m_fov = m_fov + m_fov*( spectrum[0] * 0.4f );
  }

  void Camera::_barrel_roll( std::vector<float>* spectrum ) {

    float time_elapsed = k_engine_settings->get_time_elapsed();
    bool vulkan = k_engine_settings->get_settings().m_api == kVulkan;

    {//Start the barrel roll
      const float barrel_roll_delay = 40.0f;
      float drop = ( *spectrum )[0] + ( *spectrum )[1] + ( *spectrum )[2];

      bool drop_ready = drop > 0.01f;
      bool delay_ready = ( m_last_barrel_roll + barrel_roll_delay ) < time_elapsed;
      bool effect_running = m_looping_the_loop || m_barrel_rolling;

      if( drop_ready && !effect_running && delay_ready ) {
        m_barrel_rolling = true;
        m_last_barrel_roll = time_elapsed;
      }
    }

    // Do a barrel roll
    if( m_barrel_rolling ) {

      float barrel_time = time_elapsed - m_last_barrel_roll;
      barrel_time *= 2.0f;

      float up = cosf( barrel_time );
      float side = sinf( barrel_time );

      if( vulkan )
        m_up = float3( -side, -up, side );
      else
        m_up = float3( -side, up, side );

      if( barrel_time > 2 * PI ) {

        if( vulkan )
          m_up = float3( 0.0f, -1.0f, 0.0f );
        else
          m_up = float3( 0.0f, 1.0f, 0.0f );

        m_barrel_rolling = false;
      }
    }
  }

  void Camera::_loop_the_loop( std::vector<float>* spectrum ) {

    float time_elapsed = k_engine_settings->get_time_elapsed();

    {
      const float loop_the_loop_delay = 20.0f;
      float high_note = ( *spectrum )[7] + ( *spectrum )[8] + ( *spectrum )[9];

      bool high_ready = high_note > 0.0f;
      bool delay_ready = ( m_last_loop_the_loop + loop_the_loop_delay ) < time_elapsed;
      bool effect_running = m_looping_the_loop || m_barrel_rolling;

      if( high_ready && !effect_running && delay_ready ) {
        m_looping_the_loop = true;
        m_last_loop_the_loop = time_elapsed;
      }
    }

    if( m_looping_the_loop ) {

      float looping_time = time_elapsed - m_last_loop_the_loop;
      looping_time *= 0.5f;

      static float debug_time = 0.0f;
      debug_time += 0.016f;
      looping_time = debug_time;

      float loop = sinf( looping_time );
      m_look_dir.y = loop;

      float sides = cosf( looping_time );
      m_up = float3( -loop, sides, -loop );


      if( looping_time > 2 * PI ) {
        m_up = float3( 0.0f, 1.0f, 0.0f );
        m_looping_the_loop = false;
      }
    }
  }

  Camera::~Camera() {

  }

}