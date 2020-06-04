# gmatools
tools and low level library for doing stuff with gmod addon files. code is not pretty, i wanted to keep the code as easy to write as possible. i still think its a lot cleaner than garrys gmad code. sorry garry

i chose c because its perfect for this task. it stays out of my way so i can write code quickly, and it compiles fast so i can test fast.

for every pointer given to you, they are only valid for as long as the original buffer is valid. i kept vm mapped files in mind when writing this. this way, the entire library has nothing to do with memory management: to free the whole thing, just free the original buffer or unmap the file.

the examples dont work on windows. if someone wants to port them, be my guest!

there are also some inlined high level functions that do all the pointer work for you. have fun!

# docs
~~docs? who needs docs with code this simple?~~  
yeah. i dont have docs yet. i will put some together someday though, so be patient!
