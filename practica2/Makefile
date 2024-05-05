# Compilador
CXX	=	mpicxx

# Opciones de compilación
CXXFLAGS	=	-std=c++11	-Wall

# Lista de archivos fuente
SRCS	=	1D.cc	ejercicio1.cc

# Lista de ejecutables a generar
TARGETS	=	$(SRCS:.cc=)

# Regla para la construcción de todos los ejecutables
all:	$(TARGETS)

# Regla genérica para la construcción de cada ejecutable
%:	%.cc
	$(CXX)	$(CXXFLAGS)	$<	-o	$@

# Regla para limpiar ejecutables
clean:
	rm	-f	$(TARGETS)
