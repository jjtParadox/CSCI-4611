# My solution to Assignment 2
Author: Jackson Turner

I opted to make this assignment rather efficient and used many shortcuts for the physics of the car and ball. For example, the ball bouncing off the left or right walls is done by simply inverting the X component of its velocity.

The only time I used complex vector math is when I calculate collisions between the ball and the car, and that is done thanks to some clever temporary casting of Points into Vectors. This allowed me to use the prexisting vector library to calculate the reflection normal, which made the collisions much easier to code.
