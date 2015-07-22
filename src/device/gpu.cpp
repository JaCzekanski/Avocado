#include "gpu.h"
#include <cstdio>
#include <SDL.h>
#include <SDL2_gfxPrimitives.h>
#include "../utils/file.h"
#include <cmath>

namespace device
{
	namespace gpu
	{
		uint32_t colorMean( int *colors, int n )
		{
			int r = 0, g = 0, b = 0;
			for (int i = 0; i < n; i++)
			{
				b += (colors[i] & 0xff0000) >> 16;
				g += (colors[i] & 0xff00) >> 8;
				r += (colors[i] & 0xff);
			}
			b /= n;
			g /= n;
			r /= n;

			return 0xff << 24
				| (r & 0xff)
				| (g & 0xff) << 8
				| (b & 0xff)<<16;
		}

		inline int min(int a, int b, int c)
		{
			if (a < b) return (a < c) ? a : c;
			return (b < c) ? b : c;
		}
		inline int max(int a, int b, int c)
		{
			if (a > b) return (a > c) ? a : c;
			return (b > c) ? b : c;
		}

		void swap(int &a, int &b)
		{
			int t = b;
			b = a;
			a = t;
		}

		inline float distance(int x1, int y1, int x2, int y2)
		{
			return sqrtf((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
		}

		void GPU::drawPolygon(int x[3], int y[3], int c[3])
		{
			// Make ccw
			int r0 = ((c[0] & 0xff));
			int r1 = ((c[1] & 0xff));
			int r2 = ((c[2] & 0xff));

			int g0 = (((c[0] >> 8) & 0xff));
			int g1 = (((c[1] >> 8) & 0xff));
			int g2 = (((c[2] >> 8) & 0xff));

			int b0 = (((c[0] >> 16) & 0xff));
			int b1 = (((c[1] >> 16) & 0xff));
			int b2 = (((c[2] >> 16) & 0xff));

			int y1 = y[0];
			int y2 = y[1];
			int y3 = y[2];

			int x1 = x[0];
			int x2 = x[1];
			int x3 = x[2];

			// Bounding rectangle
			int minx = (int)min(x1, x2, x3);
			int maxx = (int)max(x1, x2, x3);
			int miny = (int)min(y1, y2, y3);
			int maxy = (int)max(y1, y2, y3);


			uint32_t* colorBuffer = (uint32_t*)pixels;

			(uint8_t*&)colorBuffer += miny * stride;

			// Scan through bounding rectangle
			for (int y = miny; y < maxy; y++)
			{
				for (int x = minx; x < maxx; x++)
				{
					// When all half-space functions positive, pixel is in triangle
					if ((x1 - x2) * (y - y1) - (y1 - y2) * (x - x1) > 0 &&
						(x2 - x3) * (y - y2) - (y2 - y3) * (x - x2) > 0 &&
						(x3 - x1) * (y - y3) - (y3 - y1) * (x - x3) > 0)
					{
						int d1 = distance(x1, y1, x, y);
						int d2 = distance(x2, y2, x, y);
						int d3 = distance(x3, y3, x, y);
						float ds = (d1 + d2 + d3) / 3;

						int r =  ((d1 / ds * r0)
								+ (d2 / ds * r1)
								+ (d3 / ds * r2)) / 3 ;

						int g =  ((d1 / ds * g0)
								+ (d2 / ds * g1)
								+ (d3 / ds * g2)) / 3;

						int b =  ((d1 / ds * b0)
								+ (d2 / ds * b1)
								+ (d3 / ds * b2)) / 3;

						colorBuffer[x] = 0xff000000 | ((b & 0xff) << 16) | ((g & 0xff) << 8) | (r & 0xff);
					}
				}

				(uint8_t*&)colorBuffer += stride;
			}
		}
		
		void GPU::step()
		{
			// Calculate GPUSTAT
			GPUSTAT = (texturePageBaseX)
				| (texturePageBaseY << 4)
				| (semiTransparency << 6)
				| (texturePageColors << 8)
				| (dither24to15 << 9)
				| (drawingToDisplayArea << 10)
				| (textureDisableAllowed << 15)
				| (setMaskWhileDrawing << 11)
				| (checkMaskBeforeDraw << 12)

				| (1 << 13)

				| (displayDisable << 23)
				| (irqAcknowledge << 24)
				| (horizontalResolution1 << 17)
				| (verticalResolution << 19)
				| (videoMode << 20)
				| (colorDepth << 21)
				| (interlace << 22)
				| (horizontalResolution2 << 16)
				| (reverseFlag << 14)
				| (1 << 26) // Ready for DMA command
				| (1 << 28)
				| (odd << 31); // Ready for receive DMA block
		}

		uint8_t GPU::read(uint32_t address)
		{
			step();

			if (address >= 0 && address < 4) return GPUREAD >> (address*8);
			if (address >= 4 && address < 8) return GPUSTAT >> ((address - 4)*8);
		}

		void GPU::write(uint32_t address, uint8_t data)
		{
			if (address >= 0 && address < 4)
			{
				tmpGP0 >>= 8;
				tmpGP0 |= data << 24;

				if (++whichGP0Byte == 4)
				{
					writeGP0(tmpGP0);
					tmpGP0 = 0;
					whichGP0Byte = 0;
				}
			}
			else if (address >= 4 && address < 8)
			{
				tmpGP1 >>= 8;
				tmpGP1 |= data << 24;

				if (++whichGP1Byte == 4)
				{
					writeGP1(tmpGP1);
					tmpGP1 = 0;
					whichGP1Byte = 0;
				}
			}
			step();
		}

		void GPU::writeGP0(uint32_t data)
		{
			static bool finished = true;
			static uint32_t command = 0;
			static uint32_t argument = 0;
			static uint32_t arguments[256];
			static int currentArgument = 0;
			static int argumentCount = 0;

			static bool isManyArguments = false;

			if (finished)
			{
				command = data >> 24;
				argument = data & 0xffffff;
				finished = true;
				argumentCount = 0;
				currentArgument = 0;
				isManyArguments = false;

				if (command == 0x00) { // NOP
				}
				else if (command >= 0x20 && command < 0x40) // Polygons
				{
					bool istextureMapped = command & 4; // True - texcoords, false - no texcoords
					bool isFourVertex = command & 8; // True - 4 vertex, false - 3 vertex
					bool isShaded = command & 0x10; // True - Gouroud shading, false - flat shading

					argumentCount = (isFourVertex ? 4 : 3) * (istextureMapped ? 2 : 1) * (isShaded ? 2 : 1) - (isShaded? 1: 0); // Becouse first color is in argument (cmd+arg, not args[])
					finished = false;
				}
				else if (command >= 0x40 && command < 0x60) // Lines
				{
					isManyArguments = command & 8; // True - n points, false - 2 points (0x55555555 terminated)
					bool isShaded = (command & 0x10)>>4; // True - Gouroud shading, false - flat shading

					argumentCount = (isManyArguments ? 0xff : (isShaded ? 2 : 1) * 2);
					finished = false;
				}
				else if (command >= 0x60 && command < 0x80) // Rectangles
				{
					bool istextureMapped = command & 4; // True - texcoords, false - no texcoords
					int size = (command & 0x18)>>3; // 0 - free size, 1 - 1x1, 2 - 8x8, 3 - 16x16

					argumentCount = (size == 0 ? 2 : 1) + (istextureMapped ? 1 : 0);
					finished = false;
				}
				else if (command == 0x28) { // Monochrome four-point polygon, opaque
					argumentCount = 4;
					finished = false;
				}
				else if (command == 0xa0) { // Copy rectangle (CPU -> VRAM)
					argumentCount = 2;
					finished = false;
				}
				else if (command == 0xE1) { // Draw mode setting
					texturePageBaseX = (argument)&0xf;
					texturePageBaseY = argument>>4;
					semiTransparency = (argument&0x60)>>5;
					texturePageColors = (argument & 0x180) >> 7;
					dither24to15 = (argument & 0x200) >> 9;
					drawingToDisplayArea = (argument & 0x400) >> 10;
					textureDisable = (argument & 0x800) >> 11;
					texturedRectangleXFlip = (argument & 0x1000) >> 12;
					texturedRectangleYFlip = (argument & 0x2000) >> 13;
				}
				else if (command == 0xe2) { // Texture window setting
					textureWindowMaskX = argument & 0x1f;
					textureWindowMaskY = (argument & 0x3e0) >> 5;
					textureWindowOffsetX = (argument & 0x7c00) >> 10;
					textureWindowOffsetY = (argument & 0xf8000) >> 15;
				}
				else if (command == 0xe3) { // Drawing area top left
					drawingAreaX1 = argument & 0x3ff;
					drawingAreaY1 = (argument & 0xFFC00) >> 10;
				}
				else if (command == 0xe4) { // Drawing area bottom right
					drawingAreaX2 = argument & 0x3ff;
					drawingAreaY2 = (argument & 0xFFC00) >> 10;
				}
				else if (command == 0xe5) { // Drawing offset
					drawingOffsetX = argument & 0x7ff;
					drawingOffsetY = (argument & 0x3FF800) >> 11;
				}
				else if (command == 0xe6) { // Mask bit setting
					setMaskWhileDrawing = argument & 1;
					checkMaskBeforeDraw = (argument & 2) >> 1;
				}
				/*else */printf("GP0(0x%02x) args 0x%06x\n", command, argument);
			}
			else 
			{
				if (currentArgument < argumentCount) {
					arguments[currentArgument++] = data;
					if ( isManyArguments &&  data == 0x55555555) argumentCount = currentArgument;
				}
				if (currentArgument == argumentCount) 
				{
					finished = true;
					if (command >= 0x20 && command < 0x40) // Polygons
					{
						bool isTextureMapped = command & 4; // True - texcoords, false - no texcoords
						bool isFourVertex = command & 8; // True - 4 vertex, false - 3 vertex
						bool isShaded = command & 0x10; // True - Gouroud shading, false - flat shading

						int ptr = 0;
						int x[4], y[4];
						int color[4];
						int texcoord[4];
						for (int i = 0; i < (isFourVertex ? 4 : 3); i++)
						{
							x[i] = (arguments[ptr] & 0xffff);
							y[i] = ((arguments[ptr++] >> 16) & 0xffff);

							if (!isShaded || i == 0) color[i] = argument & 0xffffff;
							if (isTextureMapped) texcoord[i] = ptr++;
							if (isShaded && i < (isFourVertex ? 4 : 3)-1) {
								color[i+1] = arguments[ptr++];
							}
						}

						//filledTrigonColor(renderer,
						//	x[0], y[0],
						//	x[1], y[1],
						//	x[2], y[2],
						//	colorMean(color,3));
						drawPolygon(x, y, color);
						if (isFourVertex) {
							drawPolygon(x + 1, y + 1, color+1);
							//filledTrigonColor(renderer,
							//	x[1], y[1],
							//	x[2], y[2],
							//	x[3], y[3],
							//	colorMean(color+1, 3));
							//drawPolygon(x + 1, y + 1, color + 1);
						}
						swap(x[0], x[1]);
						swap(y[0], y[1]);
						swap(color[0], color[1]);
						drawPolygon(x, y, color);
					}
					else if (command >= 0x40 && command < 0x60) // Lines
					{
						isManyArguments = command & 8; // True - n points, false - 2 points (0x55555555 terminated)
						bool isShaded = command & 0x10; // True - Gouroud shading, false - flat shading

						int ptr = 0;
						int sx, sy, sc;
						int ex = 0, ey = 0, ec = 0;
						for (int i = 0; i < argumentCount; i++)
						{
							if (i == 0)
							{
								sx = arguments[ptr] & 0xffff;
								sy = (arguments[ptr++] & 0xffff0000) >> 16;
								if (isShaded) sc = arguments[ptr++];
								else sc = command & 0xffffff;
							}
							else
							{
								sx = ex;
								sy = ey;
								sc = ec;
							}

							ex = arguments[ptr] & 0xffff;
							ey = (arguments[ptr++] & 0xffff0000) >> 16;
							if (isShaded) ec = arguments[ptr++];
							else ec = command & 0xffffff;

							//lineColor(renderer, sx, sy, ex, ey, colorMean(&ec, 1));
							int x[3] = { sx, sx, ex };
							int y[3] = { ex, ex, ey };
							int c[3] = { sc, sc, sc };

							drawPolygon(x, y, c);
						}
					}
					else if (command >= 0x60 && command < 0x80) // Rectangles
					{
						bool istextureMapped = command & 4; // True - texcoords, false - no texcoords
						int size = (command & 0x18) >> 3; // 0 - free size, 1 - 1x1, 2 - 8x8, 3 - 16x16

						int width = 1;
						int height = 1;

						if (size == 2) width = height = 8;
						else if (size == 3) width = height = 16;
						else
						{
							width = arguments[(istextureMapped?2:1)] & 0xffff;
							height = (arguments[(istextureMapped ? 2 : 1)] & 0xffff0000) >> 16;
						}

						SDL_Rect r;

						r.x = arguments[0] & 0xffff;
						r.y = (arguments[0] & 0xffff0000) >> 16;
						r.w = width;
						r.h = height;

						int sc = argument;
						//boxColor(renderer, r.x, r.y, r.x+r.w, r.y+r.h, colorMean(&sc, 1));
						int x[4] = { r.x, r.x + r.w, r.x, r.x + r.w };
						int y[4] = { r.y, r.y, r.y + r.h, r.y + r.h };
						int c[3] = { sc, sc, sc };
						
						drawPolygon(x + 1, y + 1, c);
						swap(x[0], x[1]);
						swap(y[0], y[1]);
						drawPolygon(x, y, c);

					}
					else if (command == 0xA0) { // Copy rectangle ( CPU -> VRAM )
						static int size;
						size = (arguments[1] & 0xffff) * ((arguments[1] & 0xffff0000) >> 16);
						static int currentHalfWord = 0;
						static int currY = 0, currX = 0;
						if (currentArgument <= 2) {
							argumentCount = 3;
							finished = false;
							currentHalfWord = 0;
							currX = (arguments[0] & 0xffff);
							currY = (arguments[0] & 0xffff0000)>16;
						}
						else {
							currentArgument = 2;
							finished = false;

							VRAM[currY][currX * 2 + 0] = (arguments[2] & 0x000000ff);
							VRAM[currY][currX * 2 + 1] = (arguments[2] & 0x0000ff00) >> 8;
							VRAM[currY][currX * 2 + 2] = (arguments[2] & 0x00ff0000) >> 16;
							VRAM[currY][currX * 2 + 3] = (arguments[2] & 0xff000000) >> 24;
							currX += 2;
							currentHalfWord += 2;
							if (currentHalfWord >= size)
							{
								finished = true;
								void* pixels;
								int pitch;
								SDL_LockTexture(texture, NULL, &pixels, &pitch);

								uint8_t* ptr;;
								for (int y = 0; y < 512; y++)
								{
									ptr = (uint8_t*)pixels + y*pitch;
									for (int x = 0; x < pitch; x++)
									{
										*(ptr++) = VRAM[y][x];
									}
								}

								SDL_UnlockTexture(texture);

							}
							if (currX >= (arguments[0] & 0xffff) + ((arguments[1] & 0xffff)))
							{
								currX = (arguments[0] & 0xffff);
								currY++;
								if (currY >= ((arguments[0] & 0xffff0000)) + ((arguments[1] & 0xffff0000) >> 16)) {
									finished = true;

									std::vector<uint8_t> vramdump;
									vramdump.resize(2048 * 512);
									memcpy(&vramdump[0], VRAM, 2048 * 512);
									putFileContents("vram.bin", vramdump);
									//__debugbreak();
								}
								if (currY > 512)
								{
									printf("Fishy...\n");
								}
							}
						}


					}
					else
					{
						printf("Unimplemented command 0x%02x (arg count: %d)\n", command, argumentCount);
					}
				}
			}
		}

		void GPU::writeGP1(uint32_t data)
		{
			static bool finished = true;
			static uint32_t command = 0;
			static uint32_t argument = 0;

			if (finished)
			{
				command = data >> 24;
				argument = data & 0xffffff;
				finished = true;

				if (command == 0x00) { // Reset GPU
					
					irqAcknowledge = false;
					displayDisable = true;
					dmaDirection = 0;
					displayAreaStartX = displayAreaStartY = 0;
					displayRangeX1 = 0x200;
					displayRangeX2 = 0x200 + 256 * 10;
					displayRangeY1 = 0x10;
					displayRangeY2 = 0x10 + 240;
					horizontalResolution1 = 0; // 320x240 NTSC
					horizontalResolution2 = false;
					verticalResolution = 0;
					videoMode = false;

					texturePageBaseX = 0;
					texturePageBaseY = 0;
					semiTransparency = 0;
					texturePageColors = 0;
					dither24to15 = 0;
					drawingToDisplayArea = 0;
					textureDisable = 0;
					texturedRectangleXFlip = 0;
					texturedRectangleYFlip = 0;

					textureWindowMaskX = 0;
					textureWindowMaskY = 0;
					textureWindowOffsetX = 0;
					textureWindowOffsetY = 0;

					drawingAreaX1 = 0;
					drawingAreaY1 = 0;

					drawingAreaX2 = 0;
					drawingAreaY2 = 0;

					drawingOffsetX = 0;
					drawingOffsetY = 0;

					setMaskWhileDrawing = 0;
					checkMaskBeforeDraw = 0;
				}
				else if (command == 0x01) { // Reset command buffer

				}
				else if (command == 0x02) { // Acknowledge IRQ1
					irqAcknowledge = false;
				}
				else if (command == 0x03) { // Display Enable
					displayDisable = (argument & 1) ? true : false;
				}
				else if (command == 0x04) { // DMA Direction
					dmaDirection = argument & 3;
				}
				else if (command == 0x05) { // Start of display area
					displayAreaStartX = argument & 0x3ff;
					displayAreaStartY = argument >> 10;
				}
				else if (command == 0x06) { // Horizontal display range
					displayRangeX1 = argument & 0xfff;
					displayRangeX2 = argument >> 12;
				}
				else if (command == 0x07) { // Vertical display range
					displayRangeY1 = argument & 0x3ff;
					displayRangeX2 = argument >> 10;
				}
				else if (command == 0x08) { // Display mode
					horizontalResolution1 = argument & 3;
					verticalResolution = (argument & 0x04) >> 2;
					videoMode = (argument & 0x08) >> 3;
					colorDepth = (argument & 0x10) >> 4;
					interlace = (argument & 0x20) >> 5;
					horizontalResolution2 = (argument & 0x40) >> 6;
					reverseFlag = (argument & 0x80) >> 7;
				}
				else if (command == 0x09) { // Allow texture disable
					textureDisableAllowed = argument & 1;
				}
				else if (command >= 0x10 && command <= 0x1f) { // get GPU Info
					printf("GPU info request!\n");
				}
				/*else */printf("GP1(0x%02x) args 0x%06x\n", command, argument);
			}
		}

		void GPU::render()
		{


			SDL_UnlockTexture(SCREEN);
			SDL_RenderCopy(renderer, SCREEN, NULL, NULL);
			SDL_RenderPresent(renderer);
			SDL_LockTexture(SCREEN, NULL, &pixels, &stride);
		}
	}
}