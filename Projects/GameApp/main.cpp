
#include <crtdbg.h>
#include <stdexcept>

#include "GameApp/GameApp.h"

namespace
{
    /// Print exception message to the output window
    void DebugError(const std::string& msg)
    {
        const std::string s = "Error: " + msg + "\n";
        OutputDebugStringA(s.c_str());
    }

    /// Print exception message to the output window and cause a window pop-up
    void ErrorWindow(const std::string& msg)
    {
        DebugError(msg);

        MessageBoxA(nullptr, std::string(msg).c_str(), "Error", MB_OK | MB_ICONERROR | MB_SETFOREGROUND);
    }

} // namespace

// -- APPLICATION ENTRY POINT --
int main()
{
    // look for memory leaks in debug mode
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
#endif
    // outer try - preempt loading/initialization errors
    try
    {
        static constexpr int kWidth  = 1536;
        static constexpr int kHeight = 864;

        // TODO: load this from file
        GameApp::Config config =
        {
            .Title = L"Mega Murder",
            .Window = {
                .Width = kWidth,
                .Height = kHeight,
            },
        };

        GameApp game(config);

        // inner try for runtime
        try
        {
            game.Run();
            return EXIT_SUCCESS;
        }
        catch (const std::exception& e)
        {
            ErrorWindow(e.what());
            return EXIT_FAILURE;
        }
        catch (...)
        {
            ErrorWindow("Unknown error occurred!");
            return EXIT_FAILURE;
        }
    }
    catch (const std::exception& e)
    {
        DebugError(e.what());
        return EXIT_FAILURE;
    }
    catch (...)
    {
        DebugError("Unknown error occurred during startup.");
        return EXIT_FAILURE;
    }
}
