#pragma once
#include <stdint.h>
#include <SDL.h>

struct SDL_Renderer;

namespace device
{
	namespace gpu
	{

		class GPU
		{
			SDL_Texture *texture;
			SDL_Texture *SCREEN;
			void* pixels;
			int stride;

			uint8_t VRAM[512][2048];
			uint32_t fifo[16];
			uint32_t tmpGP0 = 0;
			uint32_t tmpGP1 = 0;
			int whichGP0Byte = 0;
			int whichGP1Byte = 0;

			uint32_t GPUREAD = 0;
			uint32_t GPUSTAT = 0;

			const float WIDTH = 640.f;
			const float HEIGHT = 480.f;
			
			// GP0(0xe1)
			int texturePageBaseX = 0;
			int texturePageBaseY = 0;
			int semiTransparency = 0;
			int texturePageColors = 0;
			bool dither24to15 = false;
			bool drawingToDisplayArea = false;
			bool textureDisable = false;
			bool texturedRectangleXFlip = false;
			bool texturedRectangleYFlip = false;

			// GP0(0xe2)
			int textureWindowMaskX = 0;
			int textureWindowMaskY = 0;
			int textureWindowOffsetX = 0;
			int textureWindowOffsetY = 0;

			// GP0(0xe3)
			int drawingAreaX1 = 0;
			int drawingAreaY1 = 0;

			// GP0(0xe4)
			int drawingAreaX2 = 0;
			int drawingAreaY2 = 0;

			// GP0(0xe5)
			int drawingOffsetX = 0;
			int drawingOffsetY = 0;

			// GP0(0xe6)
			int setMaskWhileDrawing = 0;
			int checkMaskBeforeDraw = 0;

			// GP1(0x02)
			bool irqAcknowledge = false;

			// GP1(0x03)
			bool displayDisable = false;

			// GP(0x04)
			int dmaDirection = 0;

			// GP1(0x05)
			int displayAreaStartX = 0;
			int displayAreaStartY = 0;

			// GP1(0x06)
			int displayRangeX1 = 0;
			int displayRangeX2 = 0;

			// GP1(0x07)
			int displayRangeY1 = 0;
			int displayRangeY2 = 0;

			// GP1(0x08)
			int horizontalResolution1 = 0;
			int verticalResolution = 0;
			bool videoMode = false;
			bool colorDepth = false;
			bool interlace = false;
			bool horizontalResolution2 = false;
			bool reverseFlag = false;
			
			// GP1(0x09)
			bool textureDisableAllowed = false;

			SDL_Renderer* renderer;
			void drawPolygon(int x[3], int y[3], int color[3]);
			void writeGP0(uint32_t data);
			void writeGP1(uint32_t data);
		public:
			bool odd = false;
			void step();
			uint8_t read(uint32_t address);
			void write(uint32_t address, uint8_t data);

			void render();
			void setRenderer(SDL_Renderer* renderer) {
				this->renderer = renderer;
				texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, 512, 1024);
				SCREEN = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
			}
		};
	}
}