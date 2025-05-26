//streamer.h
//Disk streaming for FMV player
//Bryan E. Topp <betopp@betopp.com> 2024

//Sets up streaming from the given filename.
//Returns 0 on success or a negative number on failure.
int streamer_init(const char *filename);

//Waits until the streamer has the given number of bytes ready. Returns the data location. Advances past it.
void *streamer_get(int nbytes);

//Tries to advance streaming in background if we have some time
void streamer_pump(void);
