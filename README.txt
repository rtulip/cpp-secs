
Let $TOP_DIR denote the directory containing this README file.
Let $INSTALL_DIR denote the directory into which this software is to be installed. 

To build and install the software use the commands:
    cd $INSTALL_DIR
    cmake $TOP_DIR -Btmp_cmake -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR
    cmake --build tmp_cmake --clean-first --target install

If Doxygen is installed, and you'd like to browse through the documentation in Firefox use the commands:
    cd $TOP_DIR
    doxygen Doxyfile
    firefox $TOP_DIR/docs/html/index.html
If you would rather use a browser other than Firefox, open $TOP_DIR/docs/html/index.html from 
within your favorite browser after building the documentation. 

To run a demonstration of the Pong game made with the ECS, use the commands:
    $INSTALL_DIR/bin/demo

Note: the demo will run until the GUI is closed.

Demo Controls:
    Left player Controls: W (up) and S (down)
    Right player controls: I (up) and K (down)
    Make a ball: spacebar 
    