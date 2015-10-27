#pragma once

#include "util.h"
#include "vectors.h"

#include <GL/glew.h>

#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#include <cstdint>
#include <vector>
#include <array>
#include <stdint.h>
#include <atomic>
#include <mutex>
#include <future>

namespace Ameisenfutter
{
	class Ameisenfutter;
	class MapViewer;
	class Map
	{
		friend class MapViewer;
	public:
		struct MapData
		{
			uint32_t numAnts;
			uint32_t numAntsWithFood;
			uint32_t numFood;
			uint32_t numPheromone;
			bool isNest;
		};

		Map(int width, int height);
		~Map();

		//Get the map data at the specified coordinates
		const MapData& GetPoint(ivec2 p) const;
		//Get the map data at the specified coordinates
		const MapData& GetPoint(int x, int y) const;

		//Get a reference to the map data at the specified coordinates
		MapData& GetPoint(ivec2 p);
		//Get a reference to the map data at the specified coordinates
		MapData& GetPoint(int x, int y);


		void update(int steps);


		mutable std::atomic<bool> shouldUpdateMapData = 0;

	private:
		int width, height;
		std::vector<Map::MapData> mapData;
	};
}
