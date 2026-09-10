#pragma once

#include <cstdint>

namespace Game
{
    struct GameState
    {
        enum class Stage : uint8_t
        {
            Boot,       // Created but not initialized
            Loading,    // Loading resources
            MainMenu,   // Creating a main menu
            Ready,      // Ready to start running
            Running,    // Game loop is running
            Editing,    // In level editor, not running game loop
            Paused,     // Game loop is paused, can still update UI and receive input
            Ended,      // Game loop ended, waiting for user input to exit or restart
            Exiting     // Shutting down
        };
        
        Stage stage = Stage::Boot;
        Stage lastStage = stage;


        // Helpers
        void Pause()
        {
            if (stage != Stage::Paused)
            {
                lastStage = stage;
                stage = Stage::Paused;
            }
        }

        void Resume()
        {
            if (stage == Stage::Paused)
                stage = lastStage;
        }

        // Helpers
        void ShowMenu() { stage = Stage::MainMenu; }

        void Run() { stage = Stage::Running; }
        void Edit() { stage = Stage::Editing; }
        void End() { stage = Stage::Ended; }
        void Exit() { stage = Stage::Exiting; }

        // Getters
        bool IsReady() const
        {
            return stage != Stage::Boot && stage != Stage::Loading;
        }

        bool IsRunning() const { return stage == Stage::Running; }
        bool IsMainMenu() const { return stage == Stage::MainMenu; }
        bool IsPaused()  const { return stage == Stage::Paused; }
        bool IsEditing() const { return stage == Stage::Editing; }
        bool IsEnded()   const { return stage == Stage::Ended; }
        bool IsExiting()  const { return stage == Stage::Exiting; }
    };
}