/** CSci-4611 Assignment 2:  Car Soccer
 */

#include "car_soccer.h"
#include "config.h"


CarSoccer::CarSoccer() : GraphicsApp(1024,768, "Car Soccer") {
    // Define a search path for finding data files (images and shaders)
    searchPath_.push_back(".");
    searchPath_.push_back("./data");
    searchPath_.push_back(DATA_DIR_INSTALL);
    searchPath_.push_back(DATA_DIR_BUILD);
}

CarSoccer::~CarSoccer() {
}


Vector2 CarSoccer::joystick_direction() {
    Vector2 dir;
    if (IsKeyDown(GLFW_KEY_LEFT))
        dir[0]--;
    if (IsKeyDown(GLFW_KEY_RIGHT))
        dir[0]++;
    if (IsKeyDown(GLFW_KEY_UP))
        dir[1]++;
    if (IsKeyDown(GLFW_KEY_DOWN))
        dir[1]--;
    return dir;
}


void CarSoccer::OnSpecialKeyDown(int key, int scancode, int modifiers) {
    if (key == GLFW_KEY_SPACE) {
        ball_.Reset();
        car_.Reset();
    }
}


void CarSoccer::UpdateSimulation(double timeStep) {
    bool goalScored = false;
    // Here's where you shound do your "simulation", updating the positions of the
    // car and ball as needed and checking for collisions.  Filling this routine
    // in is the main part of the assignment.
    float speed = car_.speed() * (float) timeStep * joystick_direction()[1];
    if (IsKeyDown(GLFW_KEY_LEFT_SHIFT)) {
        speed *= 2.0f;
    }
    float spdX = speed * (float) sin(car_.heading());
    float spdZ = speed * (float) cos(car_.heading());
    car_.set_position(car_.position() - Vector3(spdX, 0, spdZ));
    car_.set_heading(car_.heading() - 4 * (float) timeStep * joystick_direction()[0] * joystick_direction()[1]);

    ball_.set_position(ball_.position() + ball_.velocity() * timeStep);
    ball_.set_velocity(ball_.velocity() - Vector3(0, 80, 0) * timeStep);

    Point3 pos = car_.position();
    if (fabs(pos.x()) + car_.collision_radius() > 40) {
       if (pos.x() > 0) {
           pos[0] = 40 - car_.collision_radius();
       } else {
           pos[0] = -40 + car_.collision_radius();
       }
    }
    if (fabs(pos.z()) + car_.collision_radius() > 50) {
        if (pos.z() > 0) {
            pos[2] = 50 - car_.collision_radius();
        } else {
            pos[2] = -50 + car_.collision_radius();
        }
    }
    car_.set_position(pos);

    pos = ball_.position();
    Vector3 vel = ball_.velocity();

    Vector3 dst = Vector3((float *) pos.value_ptr()) - Vector3((float *) car_.position().value_ptr());
    if (dst.Length() < ball_.radius() + car_.collision_radius()) {
        pos = pos + dst.ToUnit() * -(dst.Length() - ball_.radius() - car_.collision_radius());
        vel = vel - 2 * vel.Dot(dst.ToUnit()) * dst.ToUnit();
        vel = vel + (Vector3(spdX, 0, spdZ) * -400);
    }

    if (fabs(pos.x()) + ball_.radius() > 40) {
        vel[0] = -vel[0] * 0.8;
        if (pos.x() > 0) {
            pos[0] = 40 - ball_.radius();
        } else {
            pos[0] = -40 + ball_.radius();
        }
    }
    if (pos.y() - ball_.radius() < 0) {
        pos[1] = 0 + ball_.radius();
        vel[1] = -vel[1] * 0.6;
    }
    if (pos.y() - ball_.radius() > 35) {
        pos[1] = 35 - ball_.radius();
        vel[1] = -vel[1] * 0.8;
    }
    if (fabs(pos.z()) + ball_.radius() > 50) {
        if (fabs(pos.x()) < 10 && pos.y() < 10)
            goalScored = true;
        vel[2] = -vel[2] * 0.8;
        if (pos.z() > 0) {
            pos[2] = 50 - ball_.radius();
        } else {
            pos[2] = -50 + ball_.radius();
        }
    }
    ball_.set_position(pos);
    ball_.set_velocity(vel);

    if (goalScored) {
        ball_.Reset();
        car_.Reset();
    }
}


void CarSoccer::InitOpenGL() {
    // Set up the camera in a good position to see the entire field
    projMatrix_ = Matrix4::Perspective(60, aspect_ratio(), 1, 1000);
    modelMatrix_ = Matrix4::LookAt(Point3(0,60,70), Point3(0,0,10), Vector3(0,1,0));
 
    // Set a background color for the screen
    glClearColor(0.8,0.8,0.8, 1);
    
    // Load some image files we'll use
    fieldTex_.InitFromFile(Platform::FindFile("pitch.png", searchPath_));
    crowdTex_.InitFromFile(Platform::FindFile("crowd.png", searchPath_));
}


void CarSoccer::DrawUsingOpenGL() {
    // Draw the crowd as a fullscreen background image
    quickShapes_.DrawFullscreenTexture(Color(1,1,1), crowdTex_);
    
    // Draw the field with the field texture on it.
    Color col(16.0/255.0, 46.0/255.0, 9.0/255.0);
    Matrix4 M = Matrix4::Translation(Vector3(0,-0.201,0)) * Matrix4::Scale(Vector3(50, 1, 60));
    quickShapes_.DrawSquare(modelMatrix_ * M, viewMatrix_, projMatrix_, col);
    M = Matrix4::Translation(Vector3(0,-0.2,0)) * Matrix4::Scale(Vector3(40, 1, 50));
    quickShapes_.DrawSquare(modelMatrix_ * M, viewMatrix_, projMatrix_, Color(1,1,1), fieldTex_);
    
    // Draw the car
    Color carcol(0.8, 0.2, 0.2);
    Matrix4 Mcar =
        Matrix4::Translation(car_.position() - Point3(0,0,0)) *
        Matrix4::RotationY(car_.heading()) *
        Matrix4::Scale(car_.size()) *
        Matrix4::Scale(Vector3(0.5,0.5,0.5));
    quickShapes_.DrawCube(modelMatrix_ * Mcar, viewMatrix_, projMatrix_, carcol);
    
    
    // Draw the ball
    Color ballcol(1,1,1);
    Matrix4 Mball =
        Matrix4::Translation(ball_.position() - Point3(0,0,0)) *
        Matrix4::Scale(Vector3(ball_.radius(), ball_.radius(), ball_.radius()));
    quickShapes_.DrawSphere(modelMatrix_ * Mball, viewMatrix_, projMatrix_, ballcol);
    
    
    // Draw the ball's shadow -- this is a bit of a hack, scaling Y by zero
    // flattens the sphere into a pancake, which we then draw just a bit
    // above the ground plane.
    Color shadowcol(0.2,0.4,0.15);
    Matrix4 Mshadow =
        Matrix4::Translation(Vector3(ball_.position()[0], -0.1, ball_.position()[2])) *
        Matrix4::Scale(Vector3(ball_.radius(), 0, ball_.radius())) *
        Matrix4::RotationX(90);
    quickShapes_.DrawSphere(modelMatrix_ * Mshadow, viewMatrix_, projMatrix_, shadowcol);
    
    
    std::vector<Point3> rBoundary;
    rBoundary.push_back(Point3( 40.0,  0.0, -50.0));
    rBoundary.push_back(Point3( 40.0,  0.0,  50.0));
    rBoundary.push_back(Point3( 40.0, 35.0,  50.0));
    rBoundary.push_back(Point3( 40.0, 35.0, -50.0));
    std::vector<Point3> lBoundary;
    lBoundary.push_back(Point3( -40.0,  0.0, -50.0));
    lBoundary.push_back(Point3( -40.0,  0.0,  50.0));
    lBoundary.push_back(Point3( -40.0, 35.0,  50.0));
    lBoundary.push_back(Point3( -40.0, 35.0, -50.0));
    std::vector<Point3> bBoundary;
    bBoundary.push_back(Point3(  40.0,  0.0, -50.0));
    bBoundary.push_back(Point3(  40.0, 35.0, -50.0));
    bBoundary.push_back(Point3( -40.0, 35.0, -50.0));
    bBoundary.push_back(Point3( -40.0,  0.0, -50.0));
    std::vector<Point3> fBoundary;
    fBoundary.push_back(Point3(  40.0,  0.0,  50.0));
    fBoundary.push_back(Point3(  40.0, 35.0,  50.0));
    fBoundary.push_back(Point3( -40.0, 35.0,  50.0));
    fBoundary.push_back(Point3( -40.0,  0.0,  50.0));
    quickShapes_.DrawLines(modelMatrix_, viewMatrix_, projMatrix_, Color(1,1,1), rBoundary, QuickShapes::LinesType::LINE_LOOP, 0.1);
    quickShapes_.DrawLines(modelMatrix_, viewMatrix_, projMatrix_, Color(1,1,1), lBoundary, QuickShapes::LinesType::LINE_LOOP, 0.1);
    quickShapes_.DrawLines(modelMatrix_, viewMatrix_, projMatrix_, Color(1,1,1), bBoundary, QuickShapes::LinesType::LINE_LOOP, 0.1);
    quickShapes_.DrawLines(modelMatrix_, viewMatrix_, projMatrix_, Color(1,1,1), fBoundary, QuickShapes::LinesType::LINE_LOOP, 0.1);

    for (int side = 0; side < 2; side++) {
        std::vector<Point3> goal;
        for (int i = -10; i <= 10; i++) {
            goal.push_back(Point3((float) i, 0.0, -50.0 + 100 * side));
            goal.push_back(Point3((float) i, 10.0, -50.0 + 100 * side));
        }
        for (int i = 0; i <= 10; i++) {
            goal.push_back(Point3(-10.0, (float) i, -50.0 + 100 * side));
            goal.push_back(Point3( 10.0, (float) i, -50.0 + 100 * side));
        }
        quickShapes_.DrawLines(modelMatrix_, viewMatrix_, projMatrix_, Color(0 + 1 * side, 1 - 1 * side, 1 - 1 * side), goal,
                               QuickShapes::LinesType::LINES, 0.1);
    }
}
