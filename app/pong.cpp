#include <GL/glut.h>
#include <iostream>
#include <ecs/world.hpp>
#include <pong/systems.hpp>
#include <pong/components.hpp>
#include <pong/resources.hpp>
#include <math.h>

using namespace pong::components;
using namespace pong::systems;
using namespace pong::resource;

int WINDOW_SIZE = 500;

// Create all the Systems for the world.
DrawSystem renderer(WINDOW_SIZE);
DrawTextSystem text_renderer(WINDOW_SIZE);
UpdateScoreTextSystem score_update_system;
MovementSystem move_sys;
PaddleWallCollisionSystem wall_sys(WINDOW_SIZE, WINDOW_SIZE);
BallWallCollisionSystem ball_wall_sys(WINDOW_SIZE, WINDOW_SIZE);
BallPaddleCollisionSystem ball_paddle_sys(M_PI / 4.0);
SpawnBallSystem spawn_ball_sys(100, 0.5, M_PI / 4.0);
KeyboardSystem keyboard_sys(1.0);
FPSSystem fps_system;
EntityCountSystem entity_count_sys;
KeyboardResource *keyboard_res;

// Create a world Registering all the components and adding resources.
auto world = ecs::world::World::create()
                 .with_component<Position>()
                 .with_component<Velocity>()
                 .with_component<Rectangle>()
                 .with_component<Color3>()
                 .with_component<Side>()
                 .with_component<Ball>()
                 .with_component<BallSpawner>()
                 .with_component<Text>()
                 .with_component<FPSCounter>()
                 .with_component<EntityCounter>()
                 .add_resource(KeyboardResource('w', 's', 'i', 'k', ' '))
                 .add_resource<ScoreResource>({0, 0})
                 .build();

/**
 * @brief Set the up world 
 * 
 * Builds all initial Entities and adds all the systems to the world.
 * 
 */
void setup_world()
{
    // Left Paddle (Red)
    world.build_entity()
        .with<Position>({-400.0, 100.0})
        .with<Velocity>({0.0, 0.0})
        .with<Rectangle>({50.0, 200.0})
        .with<Color3>({1.0f, 0.0f, 0.0f})
        .with<Side>(Side::LEFT)
        .build();

    // Right Paddle (Blue)
    world.build_entity()
        .with<Position>({350, 100})
        .with<Velocity>({0.0, 0.0})
        .with<Rectangle>({50.0, 200.0})
        .with<Color3>({0.0f, 0.0f, 1.0f})
        .with<Side>(Side::RIGHT)
        .build();

    // Left Paddle Score
    world.build_entity()
        .with<Position>({-250.0, 400.0})
        .with<Color3>({0.0, 0.0, 0.0})
        .with<Side>(Side::LEFT)
        .with<Text>({"", GLUT_BITMAP_TIMES_ROMAN_24})
        .build();

    // Right Paddle Score
    world.build_entity()
        .with<Position>({250.0, 400.0})
        .with<Color3>({0.0, 0.0, 0.0})
        .with<Side>(Side::RIGHT)
        .with<Text>({"", GLUT_BITMAP_TIMES_ROMAN_24})
        .build();

    // Ball Spawner
    world.build_entity()
        .with<BallSpawner>({})
        .build();

    // Text to instruct how to make a ball
    world.build_entity()
        .with<Position>({-250.0, -250.0})
        .with<Color3>({0.0, 0.0, 0.0})
        .with<Text>({"Press Space to make a ball!", GLUT_BITMAP_TIMES_ROMAN_24})
        .build();

    // FPS Counter
    world.build_entity()
        .with<Position>({-450.0, -450.0})
        .with<Color3>({0.0, 0.0, 0.0})
        .with<Text>({"", GLUT_BITMAP_TIMES_ROMAN_10})
        .with<FPSCounter>({FPSCounter::Clock::now()})
        .build();

    // Entity Counter
    world.build_entity()
        .with<Position>({250.0, -450.0})
        .with<Color3>({0.0, 0.0, 0.0})
        .with<Text>({"", GLUT_BITMAP_TIMES_ROMAN_10})
        .with<EntityCounter>({})
        .build();

    // Add Systems to the world.
    world.add_systems()
        .add_system(&text_renderer, "TextRenderingSystem", {})                                                               // 0
        .add_system(&renderer, "RenderingSystem", {"TextRenderingSystem"})                                                   // 1
        .add_system(&keyboard_sys, "KeyboardSystem", {"RenderingSystem"})                                                    // 2
        .add_system(&spawn_ball_sys, "SpawnBallSystem", {"KeyboardSystem"})                                                  // 3
        .add_system(&move_sys, "MovementSystem", {"SpawnBallSystem"})                                                        // 4
        .add_system(&ball_wall_sys, "BallWallCollisionSystem", {"MovementSystem"})                                           // 5
        .add_system(&wall_sys, "PaddleWallCollisionSystem", {"MovementSystem"})                                              // 5
        .add_system(&ball_paddle_sys, "BallPaddleCollisionSystem", {"PaddleWallCollisionSystem", "BallWallCollisionSystem"}) // 6
        .add_system(&score_update_system, "UpdateScoreTextSystem", {"BallWallCollisionSystem"})                              // 6
        .add_system(&entity_count_sys, "EntityCountSystem", {"BallWallCollisionSystem", "SpawnBallSystem"})                  // 6
        .add_system(&fps_system, "FPSSystem", {"BallPaddleCollisionSystem"})                                                 // 7
        .done();

    keyboard_res = world.find<KeyboardResource>()->get<KeyboardResource>(0);
}

void setup()
{
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    setup_world();
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glLoadIdentity();

    world.dispatch();

    glutSwapBuffers();
    glutPostRedisplay();
};

/**
 * @brief Callback function for handling keyboard input
 * 
 * @param key 
 */
void handle_key_press(unsigned char key, int, int)
{
    if (key == keyboard_res->PADDLE_LEFT_DOWN)
        keyboard_res->PADDLE_STATE_LEFT = KeyboardResource::PaddleState::DOWN;
    else if (key == keyboard_res->PADDLE_LEFT_UP)
        keyboard_res->PADDLE_STATE_LEFT = KeyboardResource::PaddleState::UP;
    else if (key == keyboard_res->PADDLE_RIGHT_DOWN)
        keyboard_res->PADDLE_STATE_RIGHT = KeyboardResource::PaddleState::DOWN;
    else if (key == keyboard_res->PADDLE_RIGHT_UP)
        keyboard_res->PADDLE_STATE_RIGHT = KeyboardResource::PaddleState::UP;
}

/**
 * @brief Callback function for handling keyboard release
 * 
 * @param key 
 */
void handle_key_release(unsigned char key, int, int)
{
    if (key == keyboard_res->PADDLE_LEFT_DOWN || key == keyboard_res->PADDLE_LEFT_UP)
        keyboard_res->PADDLE_STATE_LEFT = KeyboardResource::PaddleState::STILL;
    else if (key == keyboard_res->PADDLE_RIGHT_DOWN || key == keyboard_res->PADDLE_RIGHT_UP)
        keyboard_res->PADDLE_STATE_RIGHT = KeyboardResource::PaddleState::STILL;

    if (key == keyboard_res->SPAWN_BALL)
        keyboard_res->SHOULD_SPAWN_BALL = true;
}

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitWindowPosition(300, 300);
    glutInitWindowSize(WINDOW_SIZE, WINDOW_SIZE);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow("ECS Pong");

    setup();
    glutDisplayFunc(display);
    glutKeyboardFunc(handle_key_press);
    glutKeyboardUpFunc(handle_key_release);

    glutMainLoop();
}
