BEGIN {
    FS=","; 
    prev = 0;
    ok = 1;
}
NR > 1 {
    id = $1;
    if (id != prev + 1) {
        print "Error: ID " id " no es correlativo después de " prev;
        ok = 0;
    }
    prev = id;
}
END {
    if (ok) {
        print "Validación OK: IDs correlativos y sin duplicados.";
    }
}