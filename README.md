
# CPP-SECS
This repository is an implementatio of a stateful Entity Component System (ECS) in C++. There's also a demo implementation of Pong which uses the ECS. This is currently only known to be compatible in a Linux development environment.

If you'd like to see the 5 minute presentation I gave on this library and demo, you can find a recording of my presentation here: https://youtu.be/lbwLZDrIXds

Thanks for looking at my project!

## Installation Instructions.

Installing the ECS and demo is easy. Start by cloning the repository from GitHub.
```bash
git clone https://github.com/rtulip/cpp-secs.git
``` 

The demo provided has a few pre-requisites which can be installed by running the install script
```bash
cd cpp-secs
sudo ./install.sh
```

This will check the version of `CMake` before installing the `OpenGL`, `GLUT`, and `Boost` libraries which are required to compile and run the application. `Doxygen` is also installed, should you want to read the documentation.

## Running the demo.
Once the install script has been run, building and running the demo is easy. Just run the `clean_build.sh` script.
```bash
./clean_build.sh
``` 
Note: the demo will run until the GUI is closed.

Demo Controls:
    Left player Controls: W (up) and S (down)
    Right player controls: I (up) and K (down)
    Make a ball: spacebar 

## Reading the Documentation
I've made a bunch of documentation explaining how the ECS and the demo were made and how they work. You can build the documenation by running the `generate_docs.sh` script. Then opening the `docs/html/index.html` file in your favorite browser
```bash
./generate_docs.sh
firefox docs/html/index.html # Replace firefox with whichever browser you'd prefer.
```