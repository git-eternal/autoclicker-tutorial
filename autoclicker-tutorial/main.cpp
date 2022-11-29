#include <iostream>
#include <Windows.h>
#include <random>
#include <thread>
#include <mutex>

#pragma comment(lib, "winmm.lib")

#define WM_LBUTTONDOWN 0x0201
#define WM_LBUTTONUP   0x0202

namespace vars
{
	int cps{ 0 };
	HWND minecraft_window{};
}

namespace randomization
{
	// initialize our mersenne twister with a random seed based on the clock (once at system startup)
	inline std::mt19937 mersenne{ static_cast<std::mt19937::result_type>(std::time(nullptr)) };

	int random_int(int min, int max)
	{
		// create the distributiion
		std::uniform_int_distribution die{ min, max };

		// generate a random number from our global generator
		return die(mersenne);
	}

	int generate_rnd(const int& cps)
	{
		return random_int(450, 550) / cps;
	}
}

namespace nt
{
	NTSYSAPI NTSTATUS	NTAPI NtDelayExecution(
		_In_ BOOLEAN Alertable, 
		_In_opt_ PLARGE_INTEGER DelayInterval);

	inline decltype(&NtDelayExecution) pNtDelayExecution{};

	// custom sleep function to fix broken randomization
	//
	__forceinline static void sleep(std::uint64_t delay_interval)
	{
		// lambda for getting NtDelayExecution pointer
		static auto grab_nt_delay_execution = [&]() -> bool
		{
			pNtDelayExecution = reinterpret_cast<decltype(pNtDelayExecution)>(
				GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtDelayExecution"));

			std::printf("NtDelayExecution: 0x%p\n", pNtDelayExecution);
			return true;
		};
		
	  static auto _ = grab_nt_delay_execution();

		// set our periodic timer resolution to 1ms
		timeBeginPeriod(1);

		LARGE_INTEGER _delay_interval{};
		_delay_interval.QuadPart = -static_cast<LONGLONG>(delay_interval * static_cast<std::uint64_t>(10'000));
		
		pNtDelayExecution(false, &_delay_interval);

		// reset periodic timer resolution
		timeEndPeriod(1);
	}
}

void left_mouse_down()
{
	PostMessageA(vars::minecraft_window,
		WM_LBUTTONDOWN, 
		MK_LBUTTON, 
		MAKELPARAM(0, 0));

	nt::sleep(randomization::generate_rnd(vars::cps));
}

void left_mouse_up()
{
	PostMessageA(vars::minecraft_window,
		WM_LBUTTONUP,
		MK_LBUTTON,
		MAKELPARAM(0, 0));

	nt::sleep(randomization::generate_rnd(vars::cps));
}

int main()
{
	using namespace vars;

	std::cout << "Enter your desired CPS: ";
	std::cin >> cps;

	minecraft_window = FindWindowA("LWJGL", nullptr);

	if (!minecraft_window)
	{
		std::cout << "Invalid window handle!\n";
		return -1;
	}

	while (true)
	{
		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
		{
			left_mouse_down();
			left_mouse_up();
		}
		else
			nt::sleep(10);
	}
}
