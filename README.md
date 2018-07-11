AstroSimulation
=====
A simulation of the solar system built with C++, OpenGL, and ImGui. 

Features
-------

* Full UI to setup and playback a simulation.
* 3D textured objects. To-scale (if you zoom in enough). They even rotate at an accurate rate and have the appropriate axial tilt.
* Orbits!
* See it all in any unit you could want.
* Want to see what happens when Jupiter has the mass of the sun? Just save your changes and load the scenario at any time. You can even create brand new scenarios if you don't mind editing some XML.

TODO
-------
* A skybox with nebulas and other space-y stuff
* A lighting system (shadows!)
* Better camera controls and object selection 
* Create and remove objects through the UI. No more editing XML manually
* More algorithms. Currently it's just velocity verlet. I wrote Runge-Kutta back when this was in Python, and just need to port it to the c++
* Some sort of algorithm comparison tool. See exactly what the difference is between velocity verlet and RK45 with adaptive stepsize.
* debug tools
* More settings and data in the ui. Point size, color, toggle graphical features
* Learn to use Blender, make some awesome renders of objects, import them. Clouds, night side lights, bump maps
* Support for (very basic) collisions between objects.
* More objects. Comets, asteroid belt, saturn's rings, spacecraft. 
* Support for spacecraft-like movement. Acceleration, fuel. New scenarios for historical missions. 
* Trajectory optimization. Best way to get spacecraft from A to B given initial conditions and constraints on fuel, acceleration, time. Slingshots, atmospheric breaking. 

License
-------
licensed under the MIT License, see LICENSE for more information.
