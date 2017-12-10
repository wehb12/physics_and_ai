#include <ncltech\PhysicsEngine.h>
#include <ncltech\SceneManager.h>
#include <nclgl\Window.h>
#include <nclgl\NCLDebug.h>
#include <nclgl\PerfTimer.h>

#include "TestScene.h"
#include "EmptyScene.h"
#include "CUDA_BallPool.h"
#include "TargetPractise.h"
#include "SoftBodyScene.h"

// CUDA includes
#include<cuda_runtime.h>
#include<vector_types.h>

const Vector4 status_colour = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
const Vector4 status_color_debug = Vector4(1.0f, 0.6f, 1.0f, 1.0f);
const Vector4 status_colour_header = Vector4(0.8f, 0.9f, 1.0f, 1.0f);

bool draw_debug = true;
bool show_perf_metrics = false;
PerfTimer timer_total, timer_physics, timer_update, timer_render;
uint shadowCycleKey = 4;


// Program Deconstructor
//  - Releases all global components and memory
//  - Optionally prints out an error message and
//    stalls the runtime if requested.
void Quit(bool error = false, const std::string &reason = "") {
	//Release Singletons
	SceneManager::Release();
	PhysicsEngine::Release();
	GraphicsPipeline::Release();
	Window::Destroy();

	//Show console reason before exit
	if (error) {
		std::cout << reason << std::endl;
		system("PAUSE");
		exit(-1);
	}
}


// Program Initialise
//  - Generates all program wide components and enqueues all scenes
//    for the SceneManager to display
void Initialize()
{
	//Initialise the Window
	if (!Window::Initialise("Game Technologies", 1280, 800, false))
		Quit(true, "Window failed to initialise!");

	//Initialize Renderer
	GraphicsPipeline::Instance();

	//Initialise the PhysicsEngine
	PhysicsEngine::Instance();

	//Enqueue All Scenes
	SceneManager::Instance()->EnqueueScene(new TestScene("Framework Sandbox! - Show off Broadphase!"));
	SceneManager::Instance()->EnqueueScene(new TargetPractise("Target Practise"));
	SceneManager::Instance()->EnqueueScene(new CUDA_BallPool("CUDA_BallPool - GPU Acceleration"));
	SceneManager::Instance()->EnqueueScene(new SoftBodyScene("Soft Body"));
}

// Print Debug Info
//  - Prints a list of status entries to the top left
//    hand corner of the screen each frame.
void PrintStatusEntries()
{
	//Print Engine Options
	NCLDebug::AddStatusEntry(status_colour_header, "NCLTech Settings");
	NCLDebug::AddStatusEntry(status_colour, "     Physics Engine: %s (Press P to toggle)", PhysicsEngine::Instance()->IsPaused() ? "Paused  " : "Enabled ");
	NCLDebug::AddStatusEntry(status_colour, "     Monitor V-Sync: %s (Press V to toggle)", GraphicsPipeline::Instance()->GetVsyncEnabled() ? "Enabled " : "Disabled");
	NCLDebug::AddStatusEntry(status_colour, "     Octrees       : %s [O]", PhysicsEngine::Instance()->Octrees() ? "Enabled" : "Disabled");
	NCLDebug::AddStatusEntry(status_colour, "     Sphere-Sphere : %s [L]", PhysicsEngine::Instance()->SphereCheck() ? "Enabled" : "Disabled");
	NCLDebug::AddStatusEntry(status_colour, "");

	//Print Current Scene Name
	NCLDebug::AddStatusEntry(status_colour_header, "[%d/%d]: %s",
		SceneManager::Instance()->GetCurrentSceneIndex() + 1,
		SceneManager::Instance()->SceneCount(),
		SceneManager::Instance()->GetCurrentScene()->GetSceneName().c_str()
		);
	NCLDebug::AddStatusEntry(status_colour, "     \x01 T/Y to cycle or R to reload scene");

	//Print Performance Timers
	NCLDebug::AddStatusEntry(status_colour, "     FPS: %5.2f  (Press H for %s info)", 1000.f / timer_total.GetAvg(), show_perf_metrics ? "less" : "more");
	if (show_perf_metrics)
	{
		timer_total.PrintOutputToStatusEntry(status_colour, "          Total Time     :");
		timer_update.PrintOutputToStatusEntry(status_colour, "          Scene Update   :");
		timer_physics.PrintOutputToStatusEntry(status_colour, "          Physics Update :");
		timer_render.PrintOutputToStatusEntry(status_colour, "          Render Scene   :");

		NCLDebug::AddStatusEntry(status_colour, "");

		NCLDebug::AddStatusEntry(status_colour, "Collision Pairs   : %i", PhysicsEngine::Instance()->NumColPairs());
		if (PhysicsEngine::Instance()->SphereCheck())
			NCLDebug::AddStatusEntry(status_colour, "Sphere Checks     : %i", PhysicsEngine::Instance()->NumSphereChecks());
	}
	NCLDebug::AddStatusEntry(status_colour, "");

	//Physics Debug Drawing options
	uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();
	NCLDebug::AddStatusEntry(status_color_debug, "--- Debug Draw  [G] ---");
	if (draw_debug)
	{
		NCLDebug::AddStatusEntry(status_color_debug, "Constraints       : %s [Z]", (drawFlags & DEBUGDRAW_FLAGS_CONSTRAINT) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Collision Normals : %s [X]", (drawFlags & DEBUGDRAW_FLAGS_COLLISIONNORMALS) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Collision Volumes : %s [C]", (drawFlags & DEBUGDRAW_FLAGS_COLLISIONVOLUMES) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Manifolds         : %s [M]", (drawFlags & DEBUGDRAW_FLAGS_MANIFOLD) ? "Enabled " : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Octrees           : %s [O]", PhysicsEngine::Instance()->Octrees() ? "Enabled" : "Disabled");
		NCLDebug::AddStatusEntry(status_color_debug, "Sphere-Sphere     : %s [L]", PhysicsEngine::Instance()->SphereCheck() ? "Enabled" : "Disabled");

	}
	NCLDebug::AddStatusEntry(status_color_debug, "");
}


// Process Input
//  - Handles all program wide keyboard inputs for
//    things such toggling the physics engine and 
//    cycling through scenes.
void HandleKeyboardInputs()
{
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_P))
		PhysicsEngine::Instance()->SetPaused(!PhysicsEngine::Instance()->IsPaused());

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_V))
		GraphicsPipeline::Instance()->SetVsyncEnabled(!GraphicsPipeline::Instance()->GetVsyncEnabled());

	uint sceneIdx = SceneManager::Instance()->GetCurrentSceneIndex();
	uint sceneMax = SceneManager::Instance()->SceneCount();
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_Y))
		SceneManager::Instance()->JumpToScene((sceneIdx + 1) % sceneMax);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_T))
		SceneManager::Instance()->JumpToScene((sceneIdx == 0 ? sceneMax : sceneIdx) - 1);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_R))
		SceneManager::Instance()->JumpToScene(sceneIdx);

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_H))
		show_perf_metrics = !show_perf_metrics;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_G))
		draw_debug = !draw_debug;

	uint drawFlags = PhysicsEngine::Instance()->GetDebugDrawFlags();
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_Z))
		drawFlags ^= DEBUGDRAW_FLAGS_CONSTRAINT;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_X))
		drawFlags ^= DEBUGDRAW_FLAGS_COLLISIONNORMALS;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_C))
		drawFlags ^= DEBUGDRAW_FLAGS_COLLISIONVOLUMES;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_M))
		drawFlags ^= DEBUGDRAW_FLAGS_MANIFOLD;

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_O))
		PhysicsEngine::Instance()->ToggleOctrees();

	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_L))
		PhysicsEngine::Instance()->ToggleSphereCheck();

	//fire a sphere in the direction the camera is looking
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_J))
	{
		GameObject* sphere = CommonUtils::BuildSphereObject(
			"fired_sphere",					// Optional: Name
			GraphicsPipeline::Instance()->GetCamera()->GetPosition(),		// Position
			0.5f,				// Half-Dimensions
			true,				// Physics Enabled?
			0.05f,				// Physical Mass (must have physics enabled)
			true,				// Physically Collidable (has collision shape)
			true,				// Dragable by user?
			Vector4(1.0f, 1.0f, 0.0f, 1.0f));// Render color
		Matrix4 view = GraphicsPipeline::Instance()->GetCamera()->BuildViewMatrix();
		Vector3 dir = Vector3(30 * view[2], 30 * view[6], 30 * view[10]);
		sphere->Physics()->SetLinearVelocity(-dir);
		SceneManager::Instance()->GetCurrentScene()->AddGameObject(sphere);
	}

	//fire a sphere in the direction the camera is looking
	if (Window::GetKeyboard()->KeyTriggered(KEYBOARD_K))
	{
		GameObject* cuboid = CommonUtils::BuildCuboidObject(
			"fired_cube",					// Optional: Name
			GraphicsPipeline::Instance()->GetCamera()->GetPosition(),		// Position
			Vector3(0.5f, 0.5f, 0.5f),				// Half-Dimensions
			true,				// Physics Enabled?
			0.05f,				// Physical Mass (must have physics enabled)
			true,				// Physically Collidable (has collision shape)
			true,				// Dragable by user?
			Vector4(0.0f, 1.0f, 1.0f, 1.0f));// Render color
		Matrix4 view = GraphicsPipeline::Instance()->GetCamera()->BuildViewMatrix();
		Vector3 dir = Vector3(30 * view[2], 30 * view[6], 30 * view[10]);
		cuboid->Physics()->SetLinearVelocity(-dir);
		SceneManager::Instance()->GetCurrentScene()->AddGameObject(cuboid);
	}

	PhysicsEngine::Instance()->SetDebugDrawFlags(drawFlags);
}


// Program Entry Point
int main()
{
	//Initialize our Window, Physics, Scenes etc
	Initialize();
	GraphicsPipeline::Instance()->SetVsyncEnabled(false);

	Window::GetWindow().GetTimer()->GetTimedMS();

	//Create main game-loop
	while (Window::GetWindow().UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE))
	{
		//Start Timing
		float dt = Window::GetWindow().GetTimer()->GetTimedMS() * 0.001f;	//How many milliseconds since last update?
																		//Update Performance Timers (Show results every second)
		//Print Status Entries
		PrintStatusEntries();

		//Handle Keyboard Inputs
		HandleKeyboardInputs();

		timer_total.BeginTimingSection();
		timer_total.UpdateRealElapsedTime(dt);
		timer_physics.UpdateRealElapsedTime(dt);
		timer_update.UpdateRealElapsedTime(dt);
		timer_render.UpdateRealElapsedTime(dt);

		//Update Scene
		timer_update.BeginTimingSection();
		SceneManager::Instance()->GetCurrentScene()->OnUpdateScene(dt);
		timer_update.EndTimingSection();

		//Update Physics	
		timer_physics.BeginTimingSection();
		PhysicsEngine::Instance()->Update(dt);
		timer_physics.EndTimingSection();

		//Render Scene
		timer_render.BeginTimingSection();
		GraphicsPipeline::Instance()->UpdateScene(dt);
		GraphicsPipeline::Instance()->RenderScene();
		{
			//Forces synchronisation if vsync is disabled
			// - This is solely to allow accurate estimation of render time
			// - We should NEVER normally lock our render or game loop!		
		//	glClientWaitSync(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, NULL), GL_SYNC_FLUSH_COMMANDS_BIT, 1000000);
		}
		timer_render.EndTimingSection();

		

		//Finish Timing
		timer_total.EndTimingSection();		
	}

	//Cleanup
	Quit();
	return 0;
}