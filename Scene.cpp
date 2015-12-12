#include "Scene.h"
#include "scenes\BouncingCube.h"
#include "scenes\SpinningCube.h"

SceneDriver::SceneDriver()
{
	m_scenes.emplace_back(new scene::SpinningCube());
	m_scenes.emplace_back(new scene::BouncingCube());
}
