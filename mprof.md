---
title: MPROF
section: 1
header: User Manual
footer: mprof 0.0.1
date: April 26, 2021
---

# NAME

mprof - MPI Profiler

# SYNOPSIS

**mprof** *[MPI COMMAND LINE]*...

**dtop** [-v|--version]

# DESCRIPTION

**mprof** is a profiler tool, that profile *MPI* application. Currently, the
profiling work only in full *MPI* applicatin because *this is not thread safe*.

# GENERAL OPTIONS

**-v, --version**
: Display a friendly help message.

**-h, --help**
: Display a friendly help message.

# ENVIRONMENT VARIABLE

**mprof** read the *environment variable* nammed *MPROF_OPTIONS* where you can
put this list of options:

**--verbose**
: Display all call of *MPI*

**--barrier**
: Display all call of *MPI_Barrier*

**--finalize**
: Display all call of *MPI_Finalize*

# EXAMPLES

**mprof -h**
: Displays help.

**mprof mpiexec -np 4 ./a.out**
: Run the profiling in your application *a.out*

**MPROF_OPTIONS="--finalize" mprof mpiexec -np 4 ./a.out**
: Display in addition when each processus call *MPI_Finalize*

# RETURN

Return *0* on success. Else return the *MPI runtime error* of your application.

# AUTHORS

Written by *Sholde*.

# SOURCE

Contribute in the project here: <https://github.com/Sholde/mprof>

# BUGS

Submit bug reports online at: <https://github.com/Sholde/mprof/issues>

# SEE ALSO

*<mpi.h>*

