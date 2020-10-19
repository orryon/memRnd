# memRnd
it's a little multithreaded random memory access measurement console tool running under the windows x64/x86

usage : memrnd [-l] [-64] [-tNN] &lt;test-size-in-MB&gt;<br/>
&emsp;&emsp;-l (-L)     -- use large pages<br/>
&emsp;&emsp;-64         -- use 64-bit integers (selected automatically when size exceeds 16383)<br/>
&emsp;&emsp;-tNN (-Tnn) -- use multiple threads, number immediately after T|t, 0 used for detected logical cpu number, 1 by default.<br/>
&emsp;parameters order doesn't matter.
  
examples :<br/>
&emsp;memrnd 128<br/>
&emsp;&emsp;creates 128MB array for the simple thread and runs over it all in a random order, finally reports about the speeds.<br/>
&emsp;memrnd -t12 128<br/>
&emsp;&emsp;creates 12 threads and 12 128MB arrays (totally using more than 1.5GB RAM).<br/>&emsp;&emsp;reports average speeds per thread and summary for all threads.<br/>
&emsp;memrnd -t8 -l 1024<br/>
&emsp;&emsp;creates 8 threads and 8 1GB arrays (totally using more than 8GB) allocated<br/>&emsp;&emsp;in large pages memory (user mush have SeLockMemoryPrivilege or error 1300 will occurs).<br/>
&emsp;memrnd 1024 -64 -t8 -l<br/>
&emsp;&emsp;the same as above but uses int64 values (long long)<br/>
&emsp;memrnd -t0 -l 256<br/>
&emsp;&emsp;creates one thread per logical cpu reported Windows and 256MB array per thread.<br/>
 
it uses Windows multimedia timer (timeGetTime()) with accuracy about 1ms. hence using small array sizes (perhaps below 16MB) is possible but is useless.<br/>
 
      
    
