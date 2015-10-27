#include "Map.h"
#include "Ameisenfutter.h"

#include <GL/glew.h>

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <array>
#include <mutex>
#include <future>
#include <iostream>
#include <algorithm>
#include <chrono>


Ameisenfutter::Map::Map(int width, int height)
	: width{ width },
	height{ height },
	mapData(width*height)
{
	auto& middle = GetPoint(width / 2, height / 2);
	middle.isNest = true;
	middle.numAnts = 100;
}


Ameisenfutter::Map::~Map()
{
}

const Ameisenfutter::Map::MapData & Ameisenfutter::Map::GetPoint(ivec2 p) const
{
	return GetPoint(p.x, p.y);
}

const Ameisenfutter::Map::MapData & Ameisenfutter::Map::GetPoint(int x, int y) const
{
	return mapData[y * width + x];
}

Ameisenfutter::Map::MapData & Ameisenfutter::Map::GetPoint(ivec2 p)
{
	return GetPoint(p.x, p.y);
}

Ameisenfutter::Map::MapData & Ameisenfutter::Map::GetPoint(int x, int y)
{
	return mapData[y * width + x];
}


void Ameisenfutter::Map::update(int steps)
{
	if (steps > 0)
	{

		shouldUpdateMapData = true;
	}
}
