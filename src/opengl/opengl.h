#pragma once
#include <cstdint>

namespace device
{
	namespace gpu
	{
		class GPU;
	}
}

namespace opengl
{
	struct Vertex
	{
		int position[2];
		int color[3];
		int texcoord[2];
	};

	struct BlitStruct
	{
		float pos[2];
		float tex[2];
	};

	const int bufferSize = 1024 * 1024;
	const int resWidth = 640;
	const int resHeight = 480;

	void init();
	bool loadExtensions();
	bool setup();
	void render(device::gpu::GPU* gpu);
}
