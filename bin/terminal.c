#include <unistd.h>
#include <stdio.h>
int main(int argc, char** argv) {
    char* args[argc + 4];
    args[0] = "/usr/bin/alacritty";
    args[1] = "--config-file";
    args[2] = "/opt/actriz/config/alacritty/alacritty.toml";
    for (int i = 1; i < argc; i++) args[i + 2] = argv[i];
    args[argc + 2] = NULL;
    execv("/usr/bin/alacritty", args);
    perror("execv alacritty"); return 1;
}
