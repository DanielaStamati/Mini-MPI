Mini-MPI
========

App that simulates the behavior of MPI. (Available on Windows and Linux)


1. Fisiere incluse:
    main.c - echivalent cu mpirun
    mpi.c - implementeaza functiile descrise in mpi.h
    mpi.h
    mpi_err.h
    Makefile
    README

2. Implementare

    2.a) MPI_COMM_WORLD
        A fost implemetat sub forma unui vector de tupluri (rank, pid)
        si foloseste o zona de memorie partajata pentru a facilita
        accesul la acesta din fiecare proces. Procesele copil isi pot
        incepe activitatea doar dupa ce vectorul MPI_COMM_WORLD este
        creat. 
        Aceasta functionalitate este asigurata de catre semaforul
        "create". Inainte de a executa exec, fiecare proces copil
        incearca sa decrementeze semaforul respectiv (initializat cu
        0) Dupa ce s-a creat MPI_COMM_WORD se incremeteaza semaforul
        cu np, np reprezentand numarul de procese. Respectiv,
        din acest moment, procesele copil isi pot incepe executia.

    2.b)Trimitere mesaje.
        Mesajele se trimit prin intermediul unor cozi de mesaje.
        Fiecare proces are o coada de mesaje proprie din care
        preia mesajele primite. Senderul pune mesajele direct pe
        coada procesului destinatar.

    2.c)MPI_Send blocant
        Aceasta functionalitate se realizeaza prin intermediul unor
        semafoare caracteristice fiecarui proces. Odata ce s-a trimis
        un mesaj, semaforul (care e initilizat cu valoarea 0) se
        decrementeaza, in momentul in care mesajul a ajuns la
        destinatie, acelasi semafor se incrementeaza care rezulta in
        deblocarea senderului.
