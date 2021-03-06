#include "BallBehaviour.h"
#include <Rendering/Camera.h>
#include <Rendering/SpriteRenderer.h>
#include <Application.h>
#include <System/system.hpp>

#include <iostream>
#include <memory>
#include <Rendering/TrailRenderer.h>
#include "CameraController.h"

using namespace std;

void setUpScene();

int main(int argc, char **argv)
{
    try {
        Application::initialize(&argc, argv);
        
        setUpScene();
        
        Application::runMainLoop();
        Application::exit();
    }
    catch (exception exc) {
        Application::leaveMainLoop();
        Application::exit();
        cout << exc.what();
    }
    
    // todo: disabled for a while
	//cout << endl << "Press any key to exit..." << endl;
	//cin.get();

	return 0;
}

void setUpScene() {
    // BALL
    GameObject* ball = new GameObject("ball");
    BallBehaviour* ballBehaviour = new BallBehaviour(ball);
    ball->AddComponent<BallBehaviour>(ballBehaviour);
    // create renderer
    //ball->renderer = make_unique<SpriteRenderer>(ball, "sprites/ball.png");

    GameObject* trail = new GameObject("trail");
	trail->transform->setParent(ball->transform.get());
	trail->renderer = make_unique<TrailRenderer>(trail, 30, 0.05f, "sprites/checkerboard.png");
    
    // TODO: add camera
    // CAMERA
	GameObject* rotator = new GameObject("rotator");
	rotator->transform->setParent(ball->transform.get());
	rotator->AddComponent<CameraController>(new CameraController(rotator));

    GameObject* camera = new GameObject("camera");
    camera->transform->setParent(rotator->transform.get());
	camera->transform->setLocalPosition(glm::vec3(0, 1, 5));
    camera->AddComponent<Camera>();
}
