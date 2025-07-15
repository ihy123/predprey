#include <windows.h>

int main(int argc, char **argv);

int APIENTRY WinMain(HINSTANCE, HINSTANCE, PSTR cmdline, int)
{
    char *argv = NULL;
    return main(0, &argv);
}
