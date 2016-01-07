#include <cstdio>
#include <string>
#include <SDL.h>
#include <SDL_opengl.h>
#include "utils/file.h"
#include "utils/string.h"
#include "mips.h"
#include "mipsInstructions.h"
#include "psxExe.h"

#include "../externals/imgui/imgui.h"
#include "../externals/imgui/examples/sdl_opengl_example/imgui_impl_sdl.h"
#undef main
#include <memory>

bool biosLog = false;
bool disassemblyEnabled = false;
bool memoryAccessLogging = false;
bool printStackTrace = false;
uint32_t memoryDumpAddress = 0;
uint32_t htimer = 0;
uint32_t timer2 = 0;

char *_mnemonic;
std::string _disasm = "";

mips::CPU cpu;
bool cpuRunning = true;

struct Breakpoint {
    bool enabled = true;
    uint32_t location = 0;
    uint32_t instruction;  // Holds instruction if enabled
};

std::vector<Breakpoint> breakpoints;

const int cpuFrequency = 44100 * 768;
const int gpuFrequency = cpuFrequency * 11 / 7;
void IRQ(int irq) {
    // printf("IRQ%d\n", irq);
    cpu.interrupt->IRQ(irq);
}

bool isBreakpointSet(uint32_t addr) {
    for (auto b : breakpoints) {
        if (b.location == addr) {
            return true;
        }
    }
    return false;
}

void addBreakpoint(uint32_t addr) {
    for (auto b : breakpoints) {
        if (b.location == addr) {
            return;  // Already exists
        }
    }
    Breakpoint b;
    b.location = addr;
    b.instruction = cpu.readMemory32(addr);
    breakpoints.push_back(b);

    cpu.writeMemory32(addr, 0xFC000000);
    printf("Breakpoint at 0x%08x added\n", addr);
}

void removeBreakpoint(uint32_t addr) {
    for (auto it = breakpoints.begin(); it != breakpoints.end(); it++) {
        if ((*it).location == addr) {
            cpu.writeMemory32(addr, (*it).instruction);
            breakpoints.erase(it);
            printf("Breakpoint at 0x%08x removed\n", addr);
            return;
        }
    }
}

void disassemblyWindow() {
    static bool windowOpened;
    if (ImGui::Begin("Disassembly", &windowOpened, ImVec2(400.f, 600.f))) {
        if (ImGui::Button("Run")) {
            cpuRunning = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("Pause")) {
            cpuRunning = false;
        }

        float lineHeight = ImGui::GetTextLineHeight();
        int lineCount = 100;
        ImGuiListClipper clipper(lineCount, lineHeight);

        auto debugCPU = std::make_shared<mips::CPU>(cpu);
        disassemblyEnabled = true;

        uint32_t PC = debugCPU->PC;
        for (int i = 0; i < 30; i++) {
            mipsInstructions::Opcode _opcode;
            _opcode.opcode = debugCPU->readMemory32(PC);
            const auto &op = mipsInstructions::OpcodeTable[_opcode.op];
            _mnemonic = op.mnemnic;

            op.instruction(debugCPU.get(), _opcode);

            ImGui::Selectable(string_format("%c 0x%08x    %08x      %s %s", (i % 4) == 0 ? 'x' : ' ', PC, _opcode,
                                            _mnemonic, _disasm.c_str())
                                  .c_str());

            PC += 4;
        }

        disassemblyEnabled = false;

        clipper.End();
    }
    ImGui::End();
}

void renderDebugWindow(SDL_Window *debugWindow) {
    int width, height;
    SDL_GetWindowSize(debugWindow, &width, &height);

    ImGui_ImplSdl_NewFrame(debugWindow);

    disassemblyWindow();

    glViewport(0, 0, width, height);
    glClearColor(0.6f, 0.6f, 0.8f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    SDL_GL_SwapWindow(debugWindow);
}

void checkForInterrupts() {
    using namespace mips::cop0;

	if ((cpu.cop0.cause.interruptPending & 4) &&
		cpu.cop0.status.interruptEnable == Bit::set && 
		(cpu.cop0.status.interruptMask & 4)) {
        cpu.cop0.cause.exception = CAUSE::Exception::interrupt;
        //cpu.cop0.cause.interruptPending = 4;

        if (cpu.shouldJump) {
            cpu.cop0.cause.isInDelaySlot = Bit::set;
            cpu.cop0.epc = cpu.PC - 4;  // EPC - return address from trap
        } else {
            cpu.cop0.epc = cpu.PC;  // EPC - return address from trap
        }

        cpu.cop0.status.oldInterruptEnable = cpu.cop0.status.previousInterruptEnable;
        cpu.cop0.status.oldMode = cpu.cop0.status.previousMode;

        cpu.cop0.status.previousInterruptEnable = cpu.cop0.status.interruptEnable;
        cpu.cop0.status.previousMode = cpu.cop0.status.mode;

        cpu.cop0.status.interruptEnable = Bit::cleared;
        cpu.cop0.status.mode = STATUS::Mode::kernel;

        if (cpu.cop0.status.bootExceptionVectors == STATUS::BootExceptionVectors::rom)
            cpu.PC = 0xbfc00180;
        else
            cpu.PC = 0x80000080;

		printf("-%s\n", cpu.interrupt->getStatus().c_str());
    }
}

void loadExeFile(std::string exePath)
{
	auto _exe = getFileContents(exePath);
	PsxExe exe;
	if (!_exe.empty()) {
		memcpy(&exe, &_exe[0], sizeof(exe));

		for (int i = 0x800; i < _exe.size(); i++) {
			cpu.writeMemory8(exe.t_addr + i - 0x800, _exe[i]);
		}

		cpu.PC = exe.pc0;
		cpu.shouldJump = false;
	}
}

bool loadExe = false;
int main(int argc, char **argv) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Cannot init SDL\n");
        return 1;
    }
    SDL_Window *window;
    SDL_Renderer *renderer;

	window = SDL_CreateWindow("Avocado", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
	if (window == nullptr)
	{
		printf("Cannot create window\n");
		return 1;
	}

	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == nullptr) {
        printf("Cannot create  renderer\n");
        return 1;
    }

    std::string biosPath = "data/bios/SCPH1001.BIN";
    auto _bios = getFileContents(biosPath);
    if (_bios.empty()) {
        printf("Cannot open BIOS");
        return 1;
    }
    memcpy(cpu.bios, &_bios[0], _bios.size());

    // memcpy(cpu.expansion, rawData, 95232);

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_Window *debugWindow = SDL_CreateWindow("Debug", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600,
                                               SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);
    SDL_GLContext glContext = SDL_GL_CreateContext(debugWindow);
    ImGui_ImplSdl_Init(debugWindow);

    ImGuiIO &io = ImGui::GetIO();
    io.Fonts->AddFontFromFileTTF("consola.ttf", 14);

    device::gpu::GPU *gpu = new device::gpu::GPU();
    cpu.setGPU(gpu);

    int gpuLine = 0;
    int gpuDot = 0;
    bool gpuOdd = false;
    bool vblank = false;

    int cycles = 0;
    int frames = 0;

    bool doDump = false;

    gpu->setRenderer(renderer);
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderClear(renderer);
    gpu->render();

    SDL_Event event;

    while (true) {
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSdl_ProcessEvent(&event);
            if (event.type == SDL_QUIT) return 0;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_SPACE) {
                    memoryAccessLogging = !memoryAccessLogging;
                    printf("MAL %d\n", memoryAccessLogging);
                }
                if (event.key.keysym.sym == SDLK_l) loadExe = true;
                if (event.key.keysym.sym == SDLK_b) biosLog = !biosLog;
				if (event.key.keysym.sym == SDLK_c) IRQ(2);
				if (event.key.keysym.sym == SDLK_d) IRQ(3);
				if (event.key.keysym.sym == SDLK_f) cpu.cop0.status.interruptEnable = device::Bit::set;
            }
			if (event.type == SDL_DROPFILE)
			{
				loadExeFile(event.drop.file);
				SDL_free(event.drop.file);
			}
        }

		int batchSize = 1;

        if (cpuRunning) {
            checkForInterrupts();
			if (!cpu.executeInstructions(batchSize*7)) {
                printf("CPU Halted\n");
                cpuRunning = false;
            }
			cycles += batchSize*7;

            timer2++;
            if (timer2 >= 0xffff) {
                timer2 = 0;
                IRQ(6);
            }

			for (int i = 0; i < batchSize*11; i++) {
                gpuDot++;

                if (gpuDot >= 3413) {
                    gpuDot = 0;

                    htimer++;
                    if (htimer >= 0xffff) {
                        htimer = 0;
                    }

                    gpuLine++;

					if (gpuLine == 240) {
						gpu->odd = false;
						gpu->step();
						IRQ(0);
                    }
					if (gpuLine >= 263) {
                        gpuLine = 0;
                        frames++;
                        gpuOdd = !gpuOdd;
						gpu->odd = true;// gpuOdd;
                        gpu->step();

                        std::string title
							= string_format("IMASK: %s, ISTAT: %s, frame: %d, htimer: %d, cpu_cycles: %d", cpu.interrupt->getMask().c_str(), cpu.interrupt->getStatus().c_str(), frames, htimer, cycles);
                        SDL_SetWindowTitle(window, title.c_str());

                        gpu->render();
                        SDL_RenderPresent(renderer);
                    }
                }
            }
        }
        if (loadExe) {
            loadExe = false;

            std::string exePath = "data/exe/";
            char filename[128];
            printf("\nEnter exe name: ");
            scanf("%s", filename);
            exePath += filename;
			loadExeFile(exePath);
        }
        if (doDump) {
            std::vector<uint8_t> ramdump;
            ramdump.resize(2 * 1024 * 1024);
            memcpy(&ramdump[0], cpu.ram, ramdump.size());
            putFileContents("ram.bin", ramdump);
            doDump = false;
        }
    }

    SDL_Quit();
    return 0;
}