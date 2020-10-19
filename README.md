# memRnd
it's a little multithreaded random memory access measurement console tool running under the windows x64/x86

usage : memrnd [-l] [-64] [-tNN] <test-size-in-MB>
        -l (-L)     -- use large pages
        -64         -- use 64-bit integers (selected automatically when size exceeds 16383)
        -tNN (-Tnn) -- use multiple threads, number immediately after T|t, 0 used for detected logical cpu number, 1 by default.
  parameters order doesn't matter.
  
examples :
  memrnd 128
    creates 128MB array for the simple thread and runs over it all in a random order, finally reports about the speeds.
  memrnd -t12 128
    creates 12 threads and 12 128MB arrays (totally using more than 1.5GB RAM). reports average speeds per thread and summary for all threads.
  memrnd -t8 -l 1024
    creates 8 threads and 8 1GB arrays (totally using more than 8GB) allocated in large pages memory (user mush have SeLockMemoryPrivilege or error 1300 will occurs).
  memrnd 1024 -64 -t8 -l
    the same as above but uses int64 values (long long)
  memrnd -t0 -l 256
    creates one thread per logical cpu reported Windows and 256MB array per thread.
 
 it uses Windows multimedia timer (timeGetTime()) with accuracy about 1ms. hence using small array sizes (perhaps below 16MB) is possible but is useless.
 
      
    
