// Developer tool: writes the built-in exercise library to <folder>/exercise_library.json.
//
//   export_default_library <folder>
//
// It was used to create data/exercise_library.json, so the sample file always
// matches ExerciseLibrary::builtInDefaults().

#include "core/DataManager.h"
#include "core/ExerciseLibrary.h"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "usage: export_default_library <folder>\n";
        return 2;
    }

    try {
        const DataManager manager(argv[1]);
        manager.saveLibrary(ExerciseLibrary::builtInDefaults());
        std::cout << "wrote " << manager.libraryPath() << "\n";
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
