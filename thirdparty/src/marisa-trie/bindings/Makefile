ALL: swig-perl swig-python swig-ruby

swig-perl:
	swig -Wall -c++ -perl -outdir perl marisa-swig.i
	mv marisa-swig_wrap.cxx perl
	cp marisa-swig.cxx marisa-swig.h perl

swig-python:
	swig -Wall -c++ -python -outdir python marisa-swig.i
	mv marisa-swig_wrap.cxx python
	cp marisa-swig.cxx marisa-swig.h python

swig-ruby:
	swig -Wall -c++ -ruby -outdir ruby marisa-swig.i
	mv marisa-swig_wrap.cxx ruby
	cp marisa-swig.cxx marisa-swig.h ruby
