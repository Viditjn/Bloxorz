all: sample2D

sample2D: Sample_GL3_2D.cpp
	g++ -g -o sample2D Sample_GL3_2D.cpp -lglfw -lGLEW -lGL -ldl -lao -lm -lmpg123

clean:
	rm sample2D
