#pragma once

#include <chrono>
#include <memory>
#include "Geometry.h"

class ObjectIterator
{
public:
	ObjectIterator(const std::vector<geometry::Object*> & objects)
		: m_objects(objects)
	{ }

	bool HasMore() const
	{
		return m_cursor < m_objects.size();
	}

	geometry::Object * Next()
	{
		assert(HasMore());
		return m_objects[m_cursor++];
	}

private:
	std::vector<geometry::Object*> m_objects;
	std::size_t m_cursor = 0;
};

class IScene
{
public:
	virtual ~IScene() { }
	virtual void Update(long long ms) = 0;
	virtual ObjectIterator GetObjects() = 0;
};

class SceneDriver
{
public:
	SceneDriver();

	void Update(bool paused)
	{
		std::chrono::steady_clock::time_point time = std::chrono::steady_clock::now();
		long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - m_lastTime).count();

		if (! paused)
			m_scenes[m_cursor]->Update(ms);

		m_lastTime = time;
	}

	ObjectIterator GetObjects()
	{
		return m_scenes[m_cursor]->GetObjects();
	}

	void Next()
	{
		if (++m_cursor == m_scenes.size())
			m_cursor = 0;
	}

private:
	std::vector<std::unique_ptr<IScene>> m_scenes;
	std::size_t m_cursor = 0;
	std::chrono::steady_clock::time_point m_lastTime = std::chrono::steady_clock::now();
};
