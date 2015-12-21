#include "Scene.h"
#include "scenes\BouncingCube.h"
#include "scenes\SpinningCube.h"
#include "scenes\SpinningSphere.h"
#include "scenes\Bunny.h"

SceneDriver::SceneDriver()
{
	m_scenes.emplace_back(new scene::SpinningCube());
	m_scenes.emplace_back(new scene::SpinningSphere());
	m_scenes.emplace_back(new scene::BouncingCube());
	//m_scenes.emplace_back(new scene::Bunny());
}
