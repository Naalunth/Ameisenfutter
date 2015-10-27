#include "MapViewer.h"


enum MapFlags
{
	NEST = (1 << 0),
	ANT = (1 << 1),
	FOOD = (1 << 2)
};


Ameisenfutter::MapViewer::MapViewer(Map& map)
	:map{ map },
	width{ map.width },
	height{ map.height },
	internalMapArray{ sf::Quads, (size_t) width*height * 4 },
	internalMapDataVertices(width*height * 4)
{
	shader.loadFromFile("shader/grid.vtx", "shader/grid.fmt");
}

Ameisenfutter::MapViewer::~MapViewer()
{
}


void Ameisenfutter::MapViewer::draw(sf::RenderTarget & target, sf::RenderStates states) const
{
	updateInternalGLMapData();

	//sf::Texture::bind(&texture);
	glBindVertexArray(vao);
	sf::Shader::bind(&shader);
	GLuint s = shader.getNativeHandle();

	glUniformMatrix4fv(glGetUniformLocation(s, "mvp"), 1, GL_FALSE, states.transform.getMatrix());
	float tileSize = tileScreenSize > 6.0f ? ((tileScreenSize - 0.6f) / tileScreenSize) : lerp(1.0f, (float) (tileScreenSize - 0.6f) / tileScreenSize, clamp(tileScreenSize*0.5f - 2, 0, 1));
	glUniform1f(glGetUniformLocation(s, "tileSize"), tileSize);

	retrieveInternalGLMapData();
	glDrawArrays(GL_QUADS, 0, width*height * 4);
	updateInternalGLMapData();

	glBindVertexArray(0);
}


void Ameisenfutter::MapViewer::createGLStuff(sf::RenderTarget & target)
{
	const GLenum buffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, buffers);

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &vboMapData);

	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(8);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*width*height * 4, 0, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, middle)));
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, color)));

	glBindBuffer(GL_ARRAY_BUFFER, vboMapData);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLMapData)*width*height * 4, 0, GL_STREAM_DRAW);
	glVertexAttribIPointer(8, 1, GL_UNSIGNED_INT, sizeof(GLMapData), reinterpret_cast<void*>(offsetof(GLMapData, flags)));

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	updateInternalGLMapVertices();
	updateInternalGLMapVertices();
	wasGLInitialized = true;
}

void Ameisenfutter::MapViewer::cleanupGLStuff(sf::RenderTarget & target)
{
	if (!wasGLInitialized) return;
	glDeleteVertexArrays(1, &vao);
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &vboMapData);
}

void Ameisenfutter::MapViewer::setTileScreenSize(float factor)
{
	tileScreenSize = factor;
}

void Ameisenfutter::MapViewer::updateInternalGLMapVertices() const
{

	std::vector<Vertex> vertices(width*height * 4);

#pragma loop( hint_parallel(0) )
#pragma loop(ivdep)
	for (int x = 0, xmax = width; x < xmax; x++)
	{
		for (int y = 0, ymax = height; y < ymax; y++)
		{
			Vertex* quad = &vertices[(y*width + x) * 4];
			quad[0].position = { (float) x, (float) y };
			quad[1].position = { x + 1.0f, (float) y };
			quad[2].position = { x + 1.0f, y + 1.0f };
			quad[3].position = { (float) x, y + 1.0f };
			for (int i = 0; i < 4; i++)
			{
				quad[i].middle = { x + 0.5f, y + 0.5f };
				//quad[i].color = { x / (float) width, y / (float) height, 0.0f, 1.0f };
				quad[i].color = { 1.0f, 1.0f, 1.0f, 1.0f };
			}
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(Vertex)*vertices.size(), &vertices[0]);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Ameisenfutter::MapViewer::updateInternalGLMapData() const
{
	std::chrono::nanoseconds waitTime{ 1 };
	if (copyThreadFuture.valid())
	{
		if (copyThreadFuture.wait_for(waitTime) == std::future_status::timeout)
		{
			return;
		}
	}
	if (map.shouldUpdateMapData)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboMapData);
		internalMapDataVerticesPointer = reinterpret_cast<GLMapData*>(glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY));
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		if (internalMapDataVerticesPointer)
		{
			map.shouldUpdateMapData = false;
			copyThreadFuture = std::async(std::launch::async, &MapViewer::internalMapDataUpdateThread, this);
		}
	}
}

void Ameisenfutter::MapViewer::retrieveInternalGLMapData() const
{
	if (copyThreadFuture.valid())
	{
		copyThreadFuture.get();
		glBindBuffer(GL_ARRAY_BUFFER, vboMapData);
		glUnmapBuffer(GL_ARRAY_BUFFER);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

	}
}

void Ameisenfutter::MapViewer::internalMapDataUpdateThread() const
{
#pragma loop( hint_parallel(0) )
#pragma loop(ivdep)
	for (int x = 0, xmax = width; x < xmax; x++)
	{
		for (int y = 0, ymax = height; y < ymax; y++)
		{
			GLMapData* quad = &internalMapDataVerticesPointer[(y*width + x) * 4];
			const Map::MapData& mapdata = map.GetPoint(x, y);
			GLuint gldata = 0u;
			gldata += mapdata.isNest ? NEST : 0;
			gldata += mapdata.numAnts + mapdata.numAntsWithFood > 0 ? ANT : 0;
			gldata += mapdata.numFood > 0 ? FOOD : 0;
			for (int i = 0; i < 4; i++)
			{
				quad[i] = { gldata };
			}
		}
	}
}
