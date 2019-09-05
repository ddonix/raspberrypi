test:test.o
	gcc test.c -o test -lwiringPi  
jg:jg.o
	gcc jg.c -o jg -lwiringPi  
lb:lb.o
	gcc lb.c -o lb -lwiringPi  
hsfe:hsfe.o
	gcc hsfe.c -g -o hsfe -lwiringPi 
pcm_r:pcm_r.o
	gcc pcm_r.c pcmlib.o -g -o pcm_r -lwiringPi  -lasound
pcm_w1:pcm_w1.o
	gcc pcm_w1.c -g -o pcm_w1 -lwiringPi  -lasound
pcm_w2:pcm_w2.o
	gcc pcm_w2.c -g -o pcm_w2 -lwiringPi  -lasound
pcmsl:pcmsl.o 
	gcc pcmsl.c -g -o pcmsl
pcmrl:pcmrl.o pcmlib_read.o pcmlib.h
	gcc pcmrl.c pcmlib_read.c -g -o pcmrl -lasound -lwiringPi
pcm_sl:pcm_sl.o pcmlib_write.o pcmlib.h
	gcc pcm_sl.c pcmlib_write.c -g -o pcm_sl -lwiringPi  -lasound -lpthread
pcm_rl:pcm_rl.o pcmlib_read.o pcmlib.h
	gcc pcm_rl.c pcmlib_read.c -g -o pcm_rl -lwiringPi  -lasound -lpthread
pcm_rl_write_pipe:pcm_rl_write_pipe.o pcmlib.o pcmlib.h
	gcc pcm_rl_write_pipe.c pcmlib.c -g -o pcm_rl_write_pipe -lwiringPi  -lasound -lpthread
pcm_rl_read_pipe:pcm_rl_read_pipe.o pcmlib.o pcmlib.h
	gcc pcm_rl_read_pipe.c pcmlib.c -g -o pcm_rl_read_pipe -lwiringPi  -lasound -lpthread
shmwrite:shmwrite.o
	gcc shmwrite.c -g -o shmwrite -lwiringPi  -lasound
tt:tt.o 
	gcc tt.c -g -o tt -lwiringPi
dc:dc.o
	gcc dc.c -g -o dc -lwiringPi  
wav:wav.o
	gcc wav.c -g -o wav -lasound
clean:  
	rm -f test jg lb pcm pcm_rl
