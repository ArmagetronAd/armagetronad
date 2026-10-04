#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#include "ePlayer.h"
#include "tLocale.h"
#include "tCommandLine.h"

#include <unistd.h>

// manually implement main so we can do our init stuff
int main(int argc, char** argv)
{

    tCommandLineData commandLine;
    commandLine.programVersion_ = &sn_programVersion;

    // analyse command line
    // tERR_MESSAGE( "Analyzing command line." );

    // change to the root build directory
    // exe is in {builddir}/src
    if (argc > 0)
    {
        tString const program{argv[0]};

        // std::cout << program << "\n";

        // go two levels up to find build directory
        for (int i = program.Len() - 2; i >= 0; --i)
        {
            if (program[i] == '/' || program[i] == '\\')
            {
                // arrived, construct build/src dir and chdir to it
                auto const buildDir = program.SubStr(0, i);
                // std::cout << buildDir << "\n";
                auto const res = chdir(buildDir);
                std::ignore = res;

                break;
            }
        }

        {
            // must be in builddir/src now, go one level up
            auto const res = chdir("..");
            std::ignore = res;
        }

        // only give program name to command line, omit other arguments
        tString fakeLocalProgram{"./src/unit_tests"};
        char* fake_argv[2] = {&fakeLocalProgram[0], nullptr};
        commandLine.Analyse(1, fake_argv);
    }

    tLocale::Load("languages.txt");

    ePlayer::Init();

    return doctest::Context(argc, argv).run();
}
